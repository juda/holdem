#pragma once
#include <vector>

namespace holdem {

class Pot {
public:
    Pot(int chips) : m(), chips(chips) {}
    Pot(const Pot &o) : m(o.m), chips(o.chips) {}
    Pot(Pot &&o) : m(std::move(o.m)), chips(o.chips) {}

    void add(int player)
    {
        m.push_back(player);
    }

    int amount() const
    {
		return chips * m.size();
	}

	int set_chips(int c) {
		return chips = c;
	}

	const std::vector<int>& contributors() const
    {
		return m;
    }

private:
	std::vector<int> m;
	int chips;
};

}
