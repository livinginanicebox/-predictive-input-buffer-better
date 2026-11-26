#include "../src/menu_shortcut.hpp"
#include <cassert>

int main() {
    using menu_shortcut::Counter;
    using menu_shortcut::GAP_MS;

    // v2.96: the menu needs four F3 taps, not one.
    {
        Counter c;
        bool opened = false;
        c.feed(true, true, false, true, 0.0, opened);
        assert(!opened);
        c.feed(true, false, false, true, 30.0, opened);   // key-up never counts
        assert(!opened);
        c.feed(true, true, false, true, 100.0, opened);
        assert(!opened);
        c.feed(true, true, true, true, 150.0, opened);    // auto-repeat never counts
        assert(!opened);
        c.feed(true, true, false, true, 200.0, opened);
        assert(!opened);
        c.feed(true, true, false, true, 300.0, opened);   // 4th tap
        assert(opened);
        // the sequence restarts after the menu opened
        c.feed(true, true, false, true, 400.0, opened);
        assert(!opened && c.presses == 1);
    }

    // One press alone never opens it, however often you repeat it slowly.
    {
        Counter c;
        bool opened = false;
        double t = 0.0;
        for (int i = 0; i < 20; ++i, t += GAP_MS + 10.0) {
            c.feed(true, true, false, true, t, opened);
            assert(!opened);
            assert(c.presses == 1);
        }
    }

    // Other keys, key-ups and the pause menu don't start or advance the count.
    {
        Counter c;
        bool opened = false;
        c.feed(false, true, false, true, 0.0, opened);
        assert(!opened && c.presses == 0);
        c.feed(true, true, false, false, 10.0, opened);
        assert(!opened && c.presses == 0);
        c.feed(true, false, false, true, 20.0, opened);
        assert(!opened && c.presses == 0);
    }

    // Three taps + a late fourth starts over instead of opening.
    {
        Counter c;
        bool opened = false;
        c.feed(true, true, false, true, 0.0, opened);
        c.feed(true, true, false, true, 100.0, opened);
        c.feed(true, true, false, true, 200.0, opened);
        assert(!opened && c.presses == 3);
        c.feed(true, true, false, true, 200.0 + GAP_MS + 1.0, opened);
        assert(!opened && c.presses == 1);
    }

    // A clock that goes backwards doesn't count as "in time".
    {
        Counter c;
        bool opened = false;
        c.feed(true, true, false, true, 1000.0, opened);
        c.feed(true, true, false, true, 900.0, opened);
        assert(!opened && c.presses == 1);
    }
}
