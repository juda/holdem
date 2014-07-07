#pragma once
#include <cassert>
#include <cstdarg>
#include <array>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstdio>
#include "Card.h"
#include "Deck.h"
#include "IO.h"
#include "Pot.h"

namespace holdem {

class Game {
public:
    Game(IO &io, const std::vector<std::string> &names, std::vector<int> &chips, int blind)
        : io(io), names(names), chips(chips), blind(blind), n(chips.size()), dealer(0), hole_cards(n), actioned(n, false), checked(n, false), folded(n, false)
    {
        reset_current_bets();
    }

    void run()
    {
        broadcast("game starts");
        broadcast("number of players is %d", n);
        broadcast("dealer is %s", name_of(dealer));

        chips[(dealer + 1) % n] -= blind;
        current_bets[(dealer + 1) % n] = blind;
        broadcast("player %s blind bet %d", name_of(dealer + 1), blind);

        chips[(dealer + 2) % n] -= blind * 2;
        current_bets[(dealer + 2) % n] = blind * 2;
        broadcast("player %s blind bet %d", name_of(dealer + 2), blind * 2);

        for (int i = 0; i < n; i++)
        {
            hole_cards[i][0] = deck.deal();
            send(i, "hole card %c %s", hole_cards[i][0].rank, suit_of(hole_cards[i][0]));
        }

        for (int i = 0; i < n; i++)
        {
            hole_cards[i][1] = deck.deal();
            send(i, "hole card %c %s", hole_cards[i][1].rank, suit_of(hole_cards[i][1]));
        }

        // pre-flop betting round (0 community cards dealt)
        if (bet_loop())
        {
            // flop betting round (3 community cards dealt)
            deck.burn();
            deal_community_card("flop");
            deal_community_card("flop");
            deal_community_card("flop");

            if (bet_loop())
            {
                // turn betting round (4 community cards dealt)
                deck.burn();
                deal_community_card("turn");

                if (bet_loop())
                {
                    // river betting round (5 community cards dealty)
                    deck.burn();
                    deal_community_card("river");

                    if (bet_loop())
                    {
                        showdown();
                        return;
                    }
                }
            }
        }

        // only one player left
		assert( all_except_one_fold() );
		
		int winner = 0;
		for ( ; winner < n && folded[winner]; ++winner) ;
		
		declare_winner({winner});
    }

private:

	typedef std::array<int, 6> ranking_t; 
	static const int HAND_INVALID = 0;
	static const int HAND_HIGH_CARD = 1;
	static const int HAND_ONE_PAIR = 2;
	static const int HAND_TWO_PAIR = 3;
	static const int HAND_THREE_OF_A_KIND = 4;
	static const int HAND_STRAIGHT = 5;
	static const int HAND_FLUSH = 6;
	static const int HAND_FULL_HOUSE = 7;
	static const int HAND_FOUR_OF_A_KIND = 8;
	static const int HAND_STRAIGHT_FLUSH = 9;
	static const int HAND_ROYAL_FLUSH = 10;

	const char* HAND_STRING[11] = {
		"invalid",
		"high card",
		"one pair",
		"two pair",
		"three of a kind",
		"straight",
		"flush",
		"full house",
		"four of a kind",
		"straight flush",
		"royal flush",
	};

    void showdown()
    {
		std::vector<std::pair<std::array<Card, 5>, int>> hands(n);

        for (int player = 0; player < n; player++)
        {
            if (folded[player])
                continue;
 
            send(player, "showdown");

            for (int i = 0; i < 5; i++)
                receive(player, hands[player].first[i]);

			std::cerr << "Player " << player << " shows:";
			for (int i = 0; i < 5; ++i) 
				std::cerr << ' ' << hands[player].first[i].rank << ' ' << suit_of(hands[player].first[i]);
			std::cerr << std::endl;

            hands[player].second = player;
        }

        // check validity of hands
		
		std::vector<std::pair<ranking_t, int> > ranking; 

		for (int player = 0; player < n; ++player) {
			if (folded[player])
				continue;

			ranking.emplace_back(calculate_ranking(hands[player].first, hands[player].second), hands[player].second);
		}

		for (const auto& rank_id : ranking) {
			int player = rank_id.second;
			
			std::cerr << "Player " << player << std::endl;

			fprintf(stderr, "debug player %d shows %c %c %c %c %c %c %c %c %c %c , which is %d .\n",
					player, hands[player].first[0].rank, hands[player].first[0].suit,
							hands[player].first[1].rank, hands[player].first[1].suit,
							hands[player].first[2].rank, hands[player].first[2].suit,
							hands[player].first[3].rank, hands[player].first[3].suit,
							hands[player].first[4].rank, hands[player].first[4].suit,
							rank_id.first.front());

			fprintf(stderr, "player %d shows %c %s %c %s %c %s %c %s %c %s , which is %s .\n",
					player, hands[player].first[0].rank, suit_of(hands[player].first[0]),
							hands[player].first[1].rank, suit_of(hands[player].first[1]),
							hands[player].first[2].rank, suit_of(hands[player].first[2]),
							hands[player].first[3].rank, suit_of(hands[player].first[3]),
							hands[player].first[4].rank, suit_of(hands[player].first[4]),
							HAND_STRING[rank_id.first.front()]);
		}

		// compare and win pots

		std::cerr << "Comparing hands" << std::endl;
		sort(ranking.begin(), ranking.end(), 
				[](const std::pair<ranking_t, int>& lhs, const std::pair<ranking_t, int>& rhs) -> bool{
					return std::lexicographical_compare(rhs.first.begin(), rhs.first.end(), lhs.first.begin(), lhs.first.end());
				});

		for (const auto& rank_id : ranking) {
			int player = rank_id.second;
      
			broadcast("player %d shows %c %s %c %s %c %s %c %s %c %s , which is %s .",
					player, hands[player].first[0].rank, suit_of(hands[player].first[0]),
							hands[player].first[1].rank, suit_of(hands[player].first[1]),
							hands[player].first[2].rank, suit_of(hands[player].first[2]),
							hands[player].first[3].rank, suit_of(hands[player].first[3]),
							hands[player].first[4].rank, suit_of(hands[player].first[4]),
							HAND_STRING[rank_id.first.front()]);
		}

		if (ranking.front().first.front() == HAND_INVALID) {
			broadcast("All hands are invalie, no winner.");
		}

		else {
			int i = 1;
			for ( ; i < n && std::equal(ranking[0].first.begin(), ranking[0].first.end(), ranking[i].first.begin()); ++i);
			
			std::vector<int> winner;
			transform(ranking.begin(), ranking.begin() + i, back_inserter(winner),
					[](const std::pair<ranking_t, int>& rank_id) -> int {
						return rank_id.second;	
					});

			declare_winner(winner);
		}

    }

	void declare_winner(const std::vector<int>& winner) {

		int pot_sum = accumulate(pots.begin(), pots.end(), 0, [](int acc, const Pot& pot) -> int { return acc + pot.amount(); });
		int chips_won = pot_sum / winner.size();

		std::ostringstream oss;
		oss << "There are " << winner.size() << " winners:";
		for (int player : winner) {
			oss << ' ' << player;
			chips[player] += chips_won;
		}
		oss << ", each of whom wins " << chips_won << " chips."; 
		broadcast(oss.str().c_str());
	}

	ranking_t calculate_ranking(std::array<Card, 5> hand, int player) {
		std::cerr << "calculating_ranking " << player << std::endl;

		//check validity
		bool match[5] = {false, false, false, false, false};
		
		for (const auto& card : hole_cards[player]) {
			int i = 0;
			for ( ; i < 5 && (match[i] || hand[i] != card); ++i) ;
			if (i < 5) match[i] = true;
		}
		for (const auto& card : community_cards) {
			int i = 0;
			for ( ; i < 5 && (match[i] || hand[i] != card); ++i) ;
			if (i < 5) match[i] = true;
		}

		if (std::any_of(match, match + 5, [](bool matched) -> bool { return !matched; })){
			return std::move(ranking_t{HAND_INVALID});
		}
		
		// replace character rank with numerical rank
		for (auto& card : hand){
			if (isdigit(card.rank)) card.rank = card.rank - '0';
			else {
				switch (card.rank){
				case 'T':
					card.rank = 10;
					break;
				case 'J':
					card.rank = 11;
					break;
				case 'Q':
					card.rank = 12;
					break;
				case 'K':
					card.rank = 13;
					break;
				case 'A':
					card.rank = 14;
					break;
				}
			}
		}

		std::sort(hand.begin(), hand.end(), 
				[](const Card& lhs, const Card& rhs) -> bool {
					return (lhs.rank > rhs.rank) || ((lhs.rank == rhs.rank) & (lhs.suit > rhs.suit));
				});

		// check flush
		bool is_flush = std::all_of(hand.begin() + 1, hand.end(), 
				[&](const Card& lhs) -> bool {
					return lhs.suit == hand[0].suit;
				});

		// check straight
		int straight_leading = -1;
		{
			// special case of A, 2, 3, 4, 5
			if (hand[0].rank == 14 && hand[4].rank == 5 
				&& hand[1].rank == 2 && hand[2].rank == 3
				&& hand[3].rank == 4) {
				straight_leading = 5;
			}

			else {
				int i = 1;
				for ( ; i < 5 && hand[i].rank + 1 == hand[i-1].rank; ++i);
				if (i == 5) straight_leading = hand[0].rank;
			}
		}

		// check pairs, three of a kind, four of a kind
		int pair_cnt = 0;
		int pair_value[2];
		int triple_value = -1;
		int quad_value = -1;
		for (int i = 0; i < 4; ++i){
			int j = i + 1;
			for ( ; j < 5 && hand[j].rank == hand[i].rank; ++j ) ;
			
			switch ( j - i ) {
			case 2:
				pair_value[pair_cnt++] = hand[i].rank;
				break;
			case 3:
				triple_value = hand[i].rank;
				break;
			case 4:
			case 5:
				quad_value = hand[i].rank;
				break;
			default:
				continue;
			}

			i = j - 1;
		}


		// royal flush
		if (is_flush && straight_leading == 14) {
			return std::move(ranking_t{HAND_ROYAL_FLUSH}); 
		}	
		
		// straight flush
		if (is_flush && straight_leading != -1) {
			return std::move(ranking_t{HAND_STRAIGHT_FLUSH, straight_leading});
		}

		// four of a kind
		if (quad_value != -1) {
			int rem_value = (hand[0].rank == quad_value) ? (hand[4].rank) : (hand[0].rank);

			return std::move(ranking_t{HAND_FOUR_OF_A_KIND, quad_value, rem_value});
		}

		// full house
		if (triple_value != -1 && pair_cnt == 1) {
			return std::move(ranking_t{HAND_FULL_HOUSE, triple_value, pair_value[0]});
		}

		// flush
		if (is_flush) {
			return std::move(ranking_t{HAND_FLUSH, hand[0].rank, hand[1].rank, hand[2].rank, hand[3].rank, hand[4].rank});
		}

		// straight 
		if (straight_leading != -1) {
			return std::move(ranking_t{HAND_STRAIGHT, straight_leading});
		}
		
		// three of a kind
		if (triple_value != -1){
			int rem_value[2];
			int rem_cnt = 0;
			for (int i = 0; rem_cnt < 2; ++i)
				if (hand[i].rank != triple_value)
					rem_value[rem_cnt++] = hand[i].rank;

			return std::move(ranking_t{HAND_THREE_OF_A_KIND, triple_value, rem_value[0], rem_value[1]});
		}

		// two pair
		if (pair_cnt == 2) {
			int rem_value;
			for (int i = 0; ; ++i) {
				if (hand[i].rank != pair_value[0] && hand[i].rank != pair_value[1]) {
					rem_value = hand[i].rank;
					break;
				}
			}

			return std::move(ranking_t{HAND_TWO_PAIR, pair_value[0], pair_value[1], rem_value});
		}

		// one pair
		if (pair_cnt == 1) {
			int rem_value[3];
			int rem_cnt = 0;
			for (int i = 0; rem_cnt < 3; ++i){
				if (hand[i].rank != pair_value[0])
					rem_value[rem_cnt++] = hand[i].rank;
			}

			return std::move(ranking_t{HAND_ONE_PAIR, pair_value[0], rem_value[0], rem_value[1], rem_value[2]});
		}

		// high card
		return std::move(ranking_t{HAND_HIGH_CARD, hand[0].rank, hand[1].rank, hand[2].rank, hand[3].rank, hand[4].rank});
	}

    // return true if game continues
    bool bet_loop()
    {
        std::cerr << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";

        broadcast("round starts");

        for (int i = 0; i < n; i++)
            broadcast("player %s has %d chips", name_of(i), chips[i]);

        // 从庄家下一个人开始说话
        int start_player = (dealer + 1) % n;
        if (is_pre_flop_round())
            // 第一轮下注有大小盲，从大盲下一个人开始说话
            start_player = (dealer + 3) % n;

        // 上一个raise的玩家，不算大小盲
        last_raiser = -1;

        for (int player = 0; player < n; player++)
            actioned[player] = checked[player] = false;

        // 下注结束条件：
        // 0. 不考虑已经fold的人
        // 1. 每个人都说过话
        // 2. 所有人下注相同，或者all-in
        // 3. 不能再raise了

        int current_player = start_player;
        for (;;)
        {
            if (folded[current_player])
            {
                current_player = (current_player + 1) % n;
                continue;
            }

            std::cerr << "current player is " << name_of(current_player) << "\n";

            int amount = get_bet_from(current_player);
            if (amount >= 0)
                bet(current_player, amount);
            else
                fold(current_player);
            actioned[current_player] = true;

            if (all_except_one_fold())
            {
                std::cerr << "all_except_one_fold\n";
                break;
            }

            if (all_players_checked())
            {
                std::cerr << "all_players_checked\n";
                break;
            }

            do current_player = (current_player + 1) % n; while (folded[current_player]);

            std::cerr << "next player is " << name_of(current_player) << "\n";

            if (all_players_actioned() && all_bet_amounts_are_equal() && there_is_no_possible_raise(current_player))
                break;
        }

        broadcast("round ends");

        // calculate pots and contributions from current_bets
        while (!all_zero(current_bets))
        {
            int x = minimum_positive(current_bets);
            Pot pot;
            for (int player = 0; player < n; player++)
            {
                if (current_bets[player] >= x)
                {
                    current_bets[player] -= x;
                    pot.add(x, player);
                }
            }
            pots.emplace_back(pot);
        }

        // print pots and contributions
        for (const Pot &pot : pots)
        {
            std::ostringstream oss;
            for (int player : pot.contributors())
                oss << " " << name_of(player);
            std::string contributors = oss.str();
            broadcast("pot has %d chips contributed by%s", pot.amount(), contributors.c_str());
        }

        // only one player left, do not deal more cards, and do not require showdown
        if (all_except_one_fold())
            return false;

        // reset *after* the loop to keep blinds
        reset_current_bets();
        return true;
    }

    template<class Container>
    bool all_zero(const Container &v)
    {
        for (size_t i = 0; i < v.size(); i++)
            if (v[i] != 0)
                return false;
        return true;
    }

    template<class Container>
    int minimum_positive(const Container &v)
    {
        int result = 0;
        for (size_t i = 0; i < v.size(); i++)
            if (v[i] > 0 && (result == 0 || v[i] < result))
                result = v[i];
        return result;
    }

    bool all_players_checked()
    {
        for (int player = 0; player < n; player++)
        {
            if (folded[player]) continue;
            if (!checked[player]) return false;
        }
        return true;
    }

    bool all_players_actioned()
    {
        for (int player = 0; player < n; player++)
        {
            if (folded[player]) continue;
            if (!actioned[player]) return false;
        }
        return true;
    }

    int get_bet_from(int player)
    {
        send(player, "action");

        std::string message;
        receive(player, message);
        std::istringstream iss(message);

        std::string action_name;
        iss >> action_name;
        if (action_name == "bet")
        {
            int bet;
            iss >> bet;
            return bet;
        }
        else if (action_name == "check")
        {
            return 0;
        }
        else if (action_name == "fold")
        {
            return -1;
        }
        else
        {
            std::cerr << "unknown action " << action_name << "\n";
            return -1;
        }
    }

    void bet(int player, int amount)
    {
        if (chips[player] < amount)
        {
            std::cerr << "illegal bet: insufficient chips\n";
            fold(player);
        }
        else
        {
            int previous_bet = current_bets[previous_player(player)];
            int actual_bet = current_bets[player] + amount;

            if (chips[player] > amount && actual_bet < previous_bet)
            {
                std::cerr << "illegal bet: have sufficient chips but didn't bet as much as the previous player\n";
                fold(player);
            }
            else if (chips[player] > amount && actual_bet > previous_bet && actual_bet - previous_bet < blind)
            {
                std::cerr << "illegal bet: have sufficient chips but didn't raise as much as the blind\n";
                fold(player);
            }
            else
            {
                chips[player] -= amount;
                current_bets[player] += amount;

                if (amount > 0 && actual_bet == previous_bet)
                {
                    std::cerr << "player " << name_of(player) << " calls\n";
                }
                else if (amount > 0 && actual_bet > previous_bet)
                {
                    std::cerr << "player " << name_of(player) << " raises\n";
                    last_raiser = player;
                }

                if (amount == 0)
                {
                    checked[player] = true;
                    broadcast("player %s checks", name_of(player));
                }
                else
                {
                    broadcast("player %s bets %d", name_of(player), amount);
                }

                broadcast("player %s total bet is %d", name_of(player), current_bets[player]);
            }
        }
    }

    void fold(int player)
    {
        folded[player] = true;
        broadcast("player %s folds", name_of(player));
    }

    void reset_current_bets()
    {
        current_bets.resize(n);
        for (int player = 0; player < n; player++)
            current_bets[player] = 0;
    }

    void broadcast(const char *format, ...)
    {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 4096, format, args);
        io.broadcast(std::string(buffer));
        va_end(args);
    }

    void send(int player, const char *format, ...)
    {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 4096, format, args);
        io.send(player, std::string(buffer));
        va_end(args);
    }

    void receive(int player, std::string &message)
    {
        io.receive(player, message);
    }

    void receive(int player, Card &card)
    {
        std::string message;
        io.receive(player, message);
        std::istringstream iss(message);
        std::string suit;
        iss >> card.rank >> suit;
        if (suit == "club")
            card.suit = 'C';
        else if (suit == "diamond")
            card.suit = 'D';
        else if (suit == "heart")
            card.suit = 'H';
        else if (suit == "spade")
            card.suit = 'S';
        else
            std::cerr << "invalid suit " << suit << "\n";
    }

    const char *name_of(int player)
    {
        return names[player % n].c_str();
    }

    const char *suit_of(const Card &card)
    {
        return suit_of(card.suit);
    }

    const char *suit_of(char suit)
    {
        switch (suit) {
        case 'C': return "club";
        case 'D': return "diamond";
        case 'H': return "heart";
        case 'S': return "spade";
        }
        assert(false);
    }

    void deal_community_card(const char *round_name)
    {
        Card card = deck.deal();
        community_cards.emplace_back(card);
        broadcast("%s card %c %s", round_name, card.rank, suit_of(card));
    }

    // 只有一个人没有fold
    bool all_except_one_fold()
    {
        return num_folded() == n-1;
    }

    int num_folded()
    {
        int cnt = 0;
        for (int player = 0; player < n; player++)
            if (folded[player])
                cnt++;
        return cnt;
    }

    // 所有人下注相同，或者已经all-in
    bool all_bet_amounts_are_equal()
    {
        int amt = -1;
        for (int player = 0; player < n; player++)
        {
            if (folded[player])
                continue;
            else if (amt == -1)
            {
                amt = current_bets[player];
                std::cerr << "set amt=" << amt << " by " << name_of(player) << "\n";
            }
            else if (chips[player] == 0)
                continue;
            else if (amt != current_bets[player])
            {
                std::cerr << "return false because amt<>" << current_bets[player] << " by " << name_of(player) << "\n";
                return false;
            }
        }
        std::cerr << "all_bet_amounts_are_equal=" << amt << "\n";
        return true;
    }

    bool there_is_no_possible_raise(int next_to_play)
    {
        // FIXME not sure about pre-flop round
        return is_pre_flop_round() || last_raiser == next_to_play;
    }

    bool is_pre_flop_round()
    {
        return community_cards.size() == 0;
    }

    // 找前一个还没fold的玩家
    int previous_player(int player)
    {
        for (int i = 1; i < n; i++)
        {
            int p = (player - i + n) % n;
            if (folded[p]) continue;
            std::cerr << "previous player of " << name_of(player) << " is " << name_of(p) << "\n";
            return p;
        }
        assert(false);
    }

    IO &io;
    const std::vector<std::string> &names;
    std::vector<int> &chips;
    const int blind;
    const int n;
    int dealer;
    Deck deck;
    std::vector<std::array<Card, 2>> hole_cards;
    std::vector<Card> community_cards;
    std::vector<Pot> pots;
    std::vector<int> current_bets;
    std::vector<bool> actioned;
    std::vector<bool> checked;
    std::vector<bool> folded;
    int last_raiser;
};

}
