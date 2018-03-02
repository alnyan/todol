#pragma once
#include <ostream>

namespace todol {
namespace color {

template<int CODE> struct Color {
};

template<int CODE> std::ostream &operator <<(std::ostream &o,
		const Color<CODE> &c) {
	return o << "\033[" << CODE << "m";
}

static constexpr Color<0> reset;
static constexpr Color<1> bold;

static constexpr Color<30> black;
static constexpr Color<31> red;
static constexpr Color<32> green;
static constexpr Color<33> yellow;
static constexpr Color<34> blue;
static constexpr Color<35> magenta;
static constexpr Color<36> cyan;
static constexpr Color<37> white;
static constexpr Color<39> console;

static constexpr Color<90> lightblack;
static constexpr Color<91> lightred;
static constexpr Color<92> lightgreen;
static constexpr Color<93> lightyellow;
static constexpr Color<94> lightblue;
static constexpr Color<95> lightmagenta;
static constexpr Color<96> lightcyan;
static constexpr Color<97> lightwhite;

}
}
