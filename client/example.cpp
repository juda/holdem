#include "client.h"
#include "player.h"
#include "common.h"
#include <iostream>
#include <utility>

void Player::init() {}
void Player::destroy() {}

std::string Player::login_name() {
	std::string login_name;
	std::cout << "[UI] login name: ";
	std::getline(std::cin, login_name);

	return std::move(login_name);
}

void Player::login_name(std::string name) {
	std::cout << "[UI] succefully login as " << name << std::endl;
}

#define GET_BET_FROM_USER	\
	while (true) {	\
		std::string action;	\
		int amt;	\
		std::cin >> action;	\
		if (action.size() < 4) {	\
			std::cout << "[UI] invalid action, " << action << std::endl;	\
			continue;	\
		}	\
		switch (action[3]) {	\
		case 's': /*raise*/ \
			std::cin >> amt;	\
			return make_decision(RAISE, amt);	\
		case 'c': /*check*/ \
			return make_decision(CHECK);	\
		case 'l': /*call*/	\
			return make_decision(CALL);	\
		case 'd': /*fold*/	\
			return make_decision(FOLD);	\
		default:	\
			std::cout << "[UI] invalid action, " << action << std::endl;	\
		}	\
	}

#define PRINT_CARD(card) \
	std::cout << "[UI] " << rank_of(card.first) << ' ' << suit_of(card.second) << std::endl

decision_type Player::preflop() {
	GET_BET_FROM_USER	
}

decision_type Player::flop() {
	GET_BET_FROM_USER
}

decision_type Player::turn() {
	GET_BET_FROM_USER
}

decision_type Player::river() {
	GET_BET_FROM_USER
}

#undef GET_BET_FROM_USER

hand_type Player::showdown() {
	std::cout << "[UI] hole cards" << std::endl;
	for (int i = 0; i < 2; ++i) {
		PRINT_CARD(query.hole_cards()[i]);
	}

	std::cout << "[UI] community cards" << std::endl;
	const auto& community_cards = query.community_cards();
	for (int i = 0; i < 5; ++i) {
		PRINT_CARD(community_cards[i]);
	}	

	std::cout << "[UI] Choose card: (hole card: 0 - 1, community cards: 2 - 6)" << std::endl;
	hand_type hand;
	for_each(hand.begin(), hand.end(), 
			[&, this](card_type& card) {
				int id;
				std::cin >> id;
				if (id <= 1) card = query.hole_cards()[id];
					else card = community_cards[id - 2];
			});

	return std::move(hand);
}

void Player::game_end() {}

