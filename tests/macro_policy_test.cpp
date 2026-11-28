// Unit tests for the macro timing parser (v3.4). Build like the other test files:
//   g++ -std=c++23 -O1 -Wall -Wextra -o /tmp/macro_test tests/macro_policy_test.cpp && /tmp/macro_test
#include "../src/macro_policy.hpp"

#include <cassert>
#include <cstdio>
#include <string>

int main() {
    using namespace macro_policy;

    // --- numbers(...) -------------------------------------------------------------
    {
        auto v = numbers("12, 34;-56\t78");
        assert(v.size() == 4);
        assert(v[0] == 12 && v[1] == 34 && v[2] == -56 && v[3] == 78);
        assert(numbers("no digits here").empty());
        assert(numbers("").empty());
    }

    // --- parseLine: tick,down -----------------------------------------------------
    {
        Click c;
        assert(parseLine("1234,1", 240, c));
        assert(c.tick == 1234 && c.down && c.player1);

        assert(parseLine("1240,0", 240, c));
        assert(c.tick == 1240 && !c.down && c.player1);

        // 60 fps macro: 30 frames = 120 ticks at 240/s
        assert(parseLine("30,1", 60, c));
        assert(c.tick == 120 && c.down);

        // tick,down,player
        assert(parseLine("900,1,2", 240, c));
        assert(c.tick == 900 && c.down && !c.player1);

        // tick,x,y,down layout
        assert(parseLine("480,120.5,-99.25,0", 240, c));
        assert(c.tick == 480 && !c.down);

        // a single number is not a click
        assert(!parseLine("480", 240, c));
        assert(!parseLine("", 240, c));
        // negative or absurd times are rejected
        assert(!parseLine("-5,1", 240, c));
        assert(!parseLine("999999999,1", 240, c));
    }

    // --- parse(...) ---------------------------------------------------------------
    {
        std::string text =
            "# my macro\n"
            "// another comment\n"
            "\n"
            "240,1\r\n"
            "300,0\n"
            "garbage line\n"
            "480,1\n";
        int read = 0;
        auto clicks = parse(text, 240, &read);
        assert(clicks.size() == 3);
        assert(read == 3);
        assert(clicks[0].tick == 240 && clicks[0].down);
        assert(clicks[1].tick == 300 && !clicks[1].down);
        assert(clicks[2].tick == 480 && clicks[2].down);
    }

    // unsorted input comes back sorted
    {
        auto clicks = parse("900,1\n100,1\n500,0\n", 240);
        assert(clicks.size() == 3);
        assert(clicks[0].tick == 100);
        assert(clicks[1].tick == 500);
        assert(clicks[2].tick == 900);
    }

    // --- nearest(...) -------------------------------------------------------------
    {
        auto clicks = parse("1000,1\n1050,0\n1100,1,2\n", 240);

        // a press 3 ticks early finds the macro press
        assert(nearest(clicks, 997, true, true, 24) == 1000);
        // a press 3 ticks late finds it too
        assert(nearest(clicks, 1003, true, true, 24) == 1000);
        // out of the window -> nothing
        assert(nearest(clicks, 900, true, true, 24) == INT_MIN);
        // direction matters: no release at 1000
        assert(nearest(clicks, 1000, false, true, 24) == INT_MIN);
        // the release at 1050
        assert(nearest(clicks, 1046, false, true, 24) == 1050);
        // player 2 only
        assert(nearest(clicks, 1101, true, false, 24) == 1100);
        assert(nearest(clicks, 1101, true, true, 24) == INT_MIN);
        // a window of zero never matches
        assert(nearest(clicks, 1000, true, true, 0) == INT_MIN);
    }

    // nearest picks the closest of two candidates
    {
        auto clicks = parse("1000,1\n1010,1\n", 240);
        assert(nearest(clicks, 1004, true, true, 24) == 1000);
        assert(nearest(clicks, 1007, true, true, 24) == 1010);
    }

    // --- remaining(...) -----------------------------------------------------------
    {
        auto clicks = parse("100,1\n200,1\n300,1\n", 240);
        assert(remaining(clicks, 0) == 3);
        assert(remaining(clicks, 200) == 2);
        assert(remaining(clicks, 301) == 0);
    }

    std::printf("macro_policy tests passed\n");
    return 0;
}
