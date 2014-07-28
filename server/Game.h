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

const int HAND_INVALID = 0;
const int HAND_HIGH_CARD = 1;
const int HAND_ONE_PAIR = 2;
const int HAND_TWO_PAIR = 3;
const int HAND_THREE_OF_A_KIND = 4;
const int HAND_STRAIGHT = 5;
const int HAND_FLUSH = 6;
const int HAND_FULL_HOUSE = 7;
const int HAND_FOUR_OF_A_KIND = 8;
const int HAND_STRAIGHT_FLUSH = 9;
const int HAND_ROYAL_FLUSH = 10;

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

class Game {
public:
    Game(IO &io, const std::vector<std::string> &names, int init_chips)
        : io(io), names(names), chips(names.size(), init_chips), blind(1), n(names.size()), dealer(n-1), hole_cards(n), current_bets(n), actioned(n, false), checked(n, false), folded(n, false), game_cnt(0)
    {
    }

    int run(int the_blind)
    {
		if (!init_round(the_blind)) {
			return num_of_participating_players();
		}
		
		++game_cnt;

		std::cerr << std::endl;
		std::cerr << "game " << game_cnt << " starts" << std::endl;
        broadcast("game starts");

		for (int i = 0; i < n; ++i) {
			std::cerr << "Player " << i << " has " << chips[i] << " chips." << std::endl;
		}

        broadcast("number of players is %d", num_of_participating_players());
		{
			std::ostringstream oss;
			std::cerr << player_list.front();
			oss << player_list.front();
			for_each(player_list.begin() + 1, player_list.end(), 
					[&oss](int player) { std::cerr << ' ' << player; oss << ' ' << player; });
			std::cerr << std::endl;
			broadcast(oss.str().c_str());
		}
		broadcast("small blind is %d", blind);

        broadcast("dealer is %d", dealer);

		if (blind > chips[small_blind]) {
        	current_bets[small_blind] = chips[small_blind];
			chips[small_blind] = 0;
		}

		else {
			current_bets[small_blind] = blind;
			chips[small_blind] -= blind;
		}
        broadcast("player %d blind bet %d", small_blind, current_bets[small_blind]);

		if ( blind * 2  > chips[big_blind] ) {
        	current_bets[big_blind] = chips[big_blind];
        	chips[big_blind] = 0;
		}

		else {
        	current_bets[big_blind] = blind * 2;
        	chips[big_blind] -= blind * 2;
		}

        broadcast("player %d blind bet %d", big_blind, current_bets[big_blind]);

        for (auto i : player_list)
		{
            hole_cards[i][0] = deck.deal();
            send(i, "hole card %c %c", hole_cards[i][0].rank, hole_cards[i][0].suit);
        }

        for (auto i : player_list)
        {
            hole_cards[i][1] = deck.deal();
            send(i, "hole card %c %c", hole_cards[i][1].rank, hole_cards[i][1].suit);
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
                        return num_of_participating_players();
                    }
                }
            }
        }
		
		// all but one fold

		std::vector<int> winner_list(n);
		for (auto player : player_list) {
			if (!folded[player]) {
				winner_list[player] = HAND_ROYAL_FLUSH;
			}
			else {
				winner_list[player] = HAND_INVALID;
			}
		}
		
		declare_winner(winner_list);
		return num_of_participating_players();
    }

	void final_stat() {
		std::cerr << "final statistics: " << std::endl;
		std::cerr << "total games: " << game_cnt << std::endl;

		std::cerr << "final chips: " << std::endl;
		for (int i = 0; i < n; ++i) {
			std::cerr << names[i] << " has " << chips[i] << " chips" << std::endl;
		}
	}

private:

	typedef std::array<int, 6> ranking_t; 

    void showdown()
    {
		std::cerr << "start showdown" << std::endl;
		broadcast("start showdown");

		std::vector<std::pair<std::array<Card, 5>, int>> hands(n);

        for (auto player : player_list)
        {
            if (folded[player])
                continue;
 
            send(player, "showdown");

            //for (int i = 0; i < 5; i++)
            //    receive(player, hands[player].first[i]);
			{
				std::string msg;
				receive(player, msg);
				std::istringstream iss(msg);

				char rank, suit;
				for (int i = 0; i < 5; ++i) {
					iss >> rank >> suit;
					hands[player].first[i].rank = rank;
					hands[player].first[i].suit = suit;
				}
			}

            hands[player].second = player;
        }

        // check validity of hands
		
		std::cerr << "checking validity and type of hands" << std::endl;
		std::vector<std::pair<ranking_t, int> > ranking; 

		for (auto player : player_list) {
			if (folded[player])
				continue;

			ranking.emplace_back(calculate_ranking(hands[player].first, hands[player].second), hands[player].second);
		}
		
		// compare and win pots

		std::cerr << "Comparing hands" << std::endl;
		sort(ranking.begin(), ranking.end(), 
				[](const std::pair<ranking_t, int>& lhs, const std::pair<ranking_t, int>& rhs) -> bool{
					return std::lexicographical_compare(rhs.first.begin(), rhs.first.end(), lhs.first.begin(), lhs.first.end());
				});

		// broadcast showdown result
		std::vector<int> winner_list(n);
		int rank_num = n;
		std::cerr << "broadcasting showdown result" << std::endl;
		for (size_t k = 0; k < ranking.size(); ++k) {
			const auto& rank_id = ranking[k];
			int player = rank_id.second;
			
			std::ostringstream oss;
			std::cerr << "player " << name_of(player) << " shows";
			oss << "player " << player << " shows";

			for (int i = 0; i < 5; ++i) {
				std::cerr << ' ' << rank_of(hands[player].first[i].rank) << ' ' << suit_of(hands[player].first[i].suit);
				oss << ' ' << hands[player].first[i].rank << ' ' << hands[player].first[i].suit;
			}
			std::cerr << ", which is " << HAND_STRING[rank_id.first.front()] << '.' << std::endl;
			oss << ", which is " << rank_id.first.front();
			broadcast(oss.str().c_str());

			if (k!=0 && ranking[k-1].first > rank_id.first) {
				--rank_num;
			}

			winner_list[player] = rank_num;
		}

		// split pot
		declare_winner(winner_list);
    }

	void declare_winner(const std::vector<int>& winner_list) {

		std::cerr << "declare winner" << std::endl;
		broadcast("declare winner");

		/*std::cerr << "rank of player: " << std::endl;
		for (int i : player_list) {
			std::cerr << "player " << name_of(i) << " : " << winner_list[i] << std::endl;
		}*/

		int pot_num = 0;
		for (const auto& pot : pots) {
			int amount = pot.amount();
			std::vector<std::pair<int, int> > rank_num;

			for (auto player : pot.contributors()) {
				rank_num.emplace_back(winner_list[player], player);
			}

			sort(rank_num.begin(), rank_num.end(), std::greater<std::pair<int, int> >());

			size_t last_sharer = 0;
			for ( ; last_sharer < rank_num.size() && rank_num[last_sharer].first == rank_num[0].first; ++last_sharer) ;
			int amt_per_winner = amount / last_sharer;
			int remaining_part = amount - amt_per_winner * last_sharer;

			int distance_from_dealer = n;
			int the_lucky_guy = -1;
			for (size_t i = 0; i < last_sharer; ++i) {
				rank_num[i].first = amt_per_winner;
				
				int player = rank_num[i].second;
				int distance = (player - dealer + n) % n;
				if (distance < distance_from_dealer) {
					distance_from_dealer = distance;
					the_lucky_guy = i;
				}
			}
			rank_num[the_lucky_guy].first += remaining_part;	// the one who is closest to the dealer (clock-wise) gets the remaining part

			std::cerr << "pot " << pot_num << " is shared by " << last_sharer << " people" << std::endl;
			broadcast("pot %d is shared by %d players", pot_num, last_sharer);
			for (size_t i = 0; i < last_sharer; ++i) {
				chips[rank_num[i].second] += rank_num[i].first;

				std::cerr << "player " << name_of(rank_num[i].second) << " gets " << rank_num[i].first << " chips" << std::endl;
				broadcast("player %d gets %d chips", rank_num[i].second, rank_num[i].first);
			}

			++pot_num;
		}
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
			return ranking_t{HAND_INVALID};
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
			return ranking_t{HAND_ROYAL_FLUSH}; 
		}	
		
		// straight flush
		if (is_flush && straight_leading != -1) {
			return ranking_t{HAND_STRAIGHT_FLUSH, straight_leading};
		}

		// four of a kind
		if (quad_value != -1) {
			int rem_value = (hand[0].rank == quad_value) ? (hand[4].rank) : (hand[0].rank);

			return ranking_t{HAND_FOUR_OF_A_KIND, quad_value, rem_value};
		}

		// full house
		if (triple_value != -1 && pair_cnt == 1) {
			return ranking_t{HAND_FULL_HOUSE, triple_value, pair_value[0]};
		}

		// flush
		if (is_flush) {
			return ranking_t{HAND_FLUSH, hand[0].rank, hand[1].rank, hand[2].rank, hand[3].rank, hand[4].rank};
		}

		// straight 
		if (straight_leading != -1) {
			return ranking_t{HAND_STRAIGHT, straight_leading};
		}
		
		// three of a kind
		if (triple_value != -1){
			int rem_value[2];
			int rem_cnt = 0;
			for (int i = 0; rem_cnt < 2; ++i)
				if (hand[i].rank != triple_value)
					rem_value[rem_cnt++] = hand[i].rank;

			return ranking_t{HAND_THREE_OF_A_KIND, triple_value, rem_value[0], rem_value[1]};
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

			return ranking_t{HAND_TWO_PAIR, pair_value[0], pair_value[1], rem_value};
		}

		// one pair
		if (pair_cnt == 1) {
			int rem_value[3];
			int rem_cnt = 0;
			for (int i = 0; rem_cnt < 3; ++i){
				if (hand[i].rank != pair_value[0])
					rem_value[rem_cnt++] = hand[i].rank;
			}

			return ranking_t{HAND_ONE_PAIR, pair_value[0], rem_value[0], rem_value[1], rem_value[2]};
		}

		// high card
		return ranking_t{HAND_HIGH_CARD, hand[0].rank, hand[1].rank, hand[2].rank, hand[3].rank, hand[4].rank};
	}

    // return true if game continues
    bool bet_loop()
    {
        std::cerr << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";

        broadcast("round starts");

		// TODO remove these redundant messages
        for (auto i : player_list) {
			std::cerr << "player " << name_of(i) << " has " << chips[i] << " chips" << std::endl;
			broadcast("player %d has %d chips", i, chips[i]);
		}

        // 从庄家下一个人开始说话
        int now_player = (dealer_in_list + 1) % player_list.size();
        if (is_pre_flop_round())
            // 第一轮下注有大小盲，从大盲下一个人开始说话
            now_player = (dealer_in_list + 3) % player_list.size();

        // 上一个raise的玩家，不算大小盲
        last_raiser = -1;
		last_raise_amount = blind << 1;

        for (int player = 0; player < n; player++)
            actioned[player] = checked[player] = false;

        // 下注结束条件：
        // 0. 不考虑已经fold的人
        // 1. 每个人都说过话
        // 2. 所有人下注相同，或者all-in
        // 3. 不能再raise了
		
		int max_chip = 0;
        for (;;)
        {
			int current_player = player_list[now_player];
			now_player = (now_player + 1) % player_list.size();

            if (all_players_actioned() && all_bet_amounts_are_equal() && (there_is_no_possible_raise(current_player) || max_chip == 0))
                break;

            if (folded[current_player])
            {
                continue;
            }

            std::cerr << "current player is " << name_of(current_player) << "\n";
			max_chip = std::max(max_chip, chips[current_player]);

            int amount = get_bet_from(current_player);
			if (amount == -2) {
				check(current_player);
			}
			else if (amount >= 0)
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
        }

		
        // print old pots and contributions
        for (size_t i = 0; i < pots.size(); ++i)
        {
			const Pot& pot = pots[i];
            std::cerr << "pot " << i << " has " << pot.amount() << " chips contributed by";
            for (int player : pot.contributors())
                std::cerr << " " << name_of(player);
			std::cerr << std::endl;;
        } 

        // calculate pots and contributions from current_bets
        while (true)
        {
            int x = minimum_positive(current_bets);
			if (x == 0) break;

            Pot pot(x);
            for (auto player : player_list)
            {
                if (current_bets[player] >= x)
                {
                    current_bets[player] -= x;
                    pot.add(player);
                } 
            }
            pots.emplace_back(std::move(pot));

			{
				const Pot& pot = pots.back();
				std::ostringstream oss;

				std::cerr << "pot " << pots.size() - 1 << " has " << pot.amount() << " chips contributed by";
				for (int player : pot.contributors()) {
					std::cerr << " " << name_of(player);
					oss << " " << player;
				}
				std::cerr << std::endl;;
				broadcast("pot %d has %d chips contributed by%s", pots.size() - 1, pot.amount(), oss.str().c_str());
			}
		}

        broadcast("round ends");

        // only one player left, do not deal more cards, and do not require showdown
        if (all_except_one_fold())
            return false;

        // reset *after* the loop to keep blinds
        // reset_current_bets();	// no need to reset current_bets again
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
        for (auto player : player_list)
        {
            if (folded[player]) continue;
            if (!checked[player]) return false;
        }
        return true;
    }

    bool all_players_actioned()
    {
        for (auto player : player_list)
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
            return -2;
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
		assert(amount >= 0);

        if (chips[player] < amount)
        {
            std::cerr << "illegal bet: insufficient chips\n";
            fold(player);
        }
        else
        {
            int previous_bet = *std::max_element(current_bets.begin(), current_bets.end()); 
            int actual_bet = current_bets[player] + amount;

            if (chips[player] > amount && actual_bet < previous_bet)
            {
                std::cerr << "illegal bet: have sufficient chips but didn't bet as much as the previous player\n";
                fold(player);
            }

            else if (chips[player] > amount && actual_bet > previous_bet && actual_bet - previous_bet < last_raise_amount)
            {
                std::cerr << "illegal bet: have sufficient chips but didn't raise as much as the last raised\n";
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
					last_raise_amount = std::max(last_raise_amount, actual_bet - previous_bet);
                }

                broadcast("player %d bets %d", player, amount);
            }
        }
    }

    void fold(int player)
    {
        folded[player] = true;
		std::cerr << "player " << name_of(player) << " folds" << std::endl;
        broadcast("player %d folds", player);
    }

	void check(int player) {
		if (last_raiser != -1 || is_pre_flop_round()) {
			std::cerr << "illegal check: there are active bets" << std::endl;
			fold(player);
			return ;
		}

		checked[player] = true;
		std::cerr << "player " << name_of(player) << " checks" << std::endl;
		broadcast("player %d checks", player);
	}

    void reset_current_bets()
    {
		std::fill(current_bets.begin(), current_bets.end(), 0);
	}

    void broadcast(const char *format, ...)
    {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 4096, format, args);

		std::cerr << "[broadcast] " << buffer << std::endl;

        io.broadcast(std::string(buffer));
        va_end(args);
    }

    void send(int player, const char *format, ...)
    {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 4096, format, args);
		
		std::cerr << "[send to " << name_of(player) << "] " << buffer << std::endl;

        io.send(player, std::string(buffer));
        va_end(args);
    }

    void receive(int player, std::string &message)
    {
        io.receive(player, message);
		std::cerr << "[receive from " << name_of(player) << "] " << message << std::endl;
	}

	// Deprecated
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
		// FIXME If this assertion always holds, the modular can be safely removed.
		assert(player < n);
        return names[player % n].c_str();
    }

	static const char* rank_of(char rank) {
		switch (rank) {
		case '2':
			return "2";
		case '3':
			return "3";
		case '4':
			return "4";
		case '5':
			return "5";
		case '6':
			return "6";
		case '7':
			return "7";
		case '8':
			return "8";
		case '9':
			return "9";
		case 'T':
			return "10";
		case 'J':
			return "J";
		case 'Q':
			return "Q";
		case 'K':
			return "K";
		case 'A':
			return "A";
		default:
			return "?";
		}
	}

	static const char* suit_of(char suit) {
		switch (suit) {
		case 'S':
			return "spade";
		case 'C':
			return "club";
		case 'H':
			return "heart";
		case 'D':
			return "diamond";
		default:
			return "?";
		}
	}

	// Deprecated
    /*const char *suit_of(const Card &card)
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
    }*/

    void deal_community_card(const char *round_name)
    {
        Card card = deck.deal();
        community_cards.emplace_back(card);
        broadcast("%s card %c %c", round_name, card.rank, card.suit);
    }

    // 只有一个人没有fold
    bool all_except_one_fold()
    {
        return (size_t)(num_folded() + 1) == player_list.size();
    }

    int num_folded()
    {
        int cnt = 0;
        for (auto player : player_list)
            if (folded[player])
                cnt++;
        return cnt;
    }

    // 所有人下注相同，或者已经all-in
    bool all_bet_amounts_are_equal()
    {
        int amt = *std::max_element(current_bets.begin(), current_bets.end());
        for (auto player : player_list)
        {
            if (!folded[player] && chips[player] != 0 && amt != current_bets[player]) {
		//		std::cerr << "all_bet_amounts_are_equal returns false because amt = " << amt
		//			<< " but player " << name_of(player) << " only bets " << current_bets[player] << std::endl;
				return false;
			}
        }
        //std::cerr << "all_bet_amounts_are_equal returns true, amt =" << amt << "\n";
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
	// Deprecated
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

	int find_next_participating_player(int dealer) {
		int ret;
		for (ret = (dealer + 1) % n; ret != dealer && chips[ret] == 0; ret = (ret + 1) % n) ;
		return (ret == dealer) ? -1 : ret;
	}

	bool init_round(int b) {
		blind = b;

		player_list.clear();
		for (int i = 0; i < n; ++i)
			if (chips[i] > 0)
				player_list.push_back(i);

		if (player_list.size() == 0) {
			std::cerr << "[ERROR] no players left" << std::endl;
			return false;
		}

		if (player_list.size() == 1) {
			std::cerr << "[NOTE] only one player is left" << std::endl;
			return false;
		}

		dealer = find_next_participating_player(dealer);
		dealer_in_list = std::find(player_list.begin(), player_list.end(), dealer) - player_list.begin();

		small_blind = player_list[(dealer_in_list + 1) % num_of_participating_players()];
		if (chips[small_blind] < blind) {
			std::cerr << "[NOTE] player " << names[small_blind] << " has only " << chips[small_blind] <<
				", who has to go all in for the small blind"<< blind << std::endl;
		}

		big_blind = player_list[(dealer_in_list + 2) % num_of_participating_players()];
		if (chips[big_blind] < 2 * blind) {
			std::cerr << "[NOTE] player " << names[big_blind] << " has only " << chips[big_blind] <<
				", who has to go all in for the big blind" << 2 * blind << std::endl;
		}

		reset_current_bets();
		deck.shuffle();
		std::fill(folded.begin(), folded.end(), false);
		pots.clear();
		community_cards.clear();

		return true;
	}

	int num_of_participating_players() {
		return player_list.size();
	}

    IO &io;
    const std::vector<std::string> &names;
    std::vector<int> chips;
	int blind;
    const int n;
    int dealer;
	int dealer_in_list;
	int small_blind;
	int big_blind;
    Deck deck;
    std::vector<std::array<Card, 2>> hole_cards;
    std::vector<Card> community_cards;
    std::vector<Pot> pots;
    std::vector<int> current_bets;
    std::vector<bool> actioned;
    std::vector<bool> checked;
    std::vector<bool> folded;
    int last_raiser;
	int last_raise_amount;
	std::vector<int> player_list;
	int game_cnt;
};

}
