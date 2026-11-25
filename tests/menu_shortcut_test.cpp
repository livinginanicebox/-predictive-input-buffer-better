#include "../src/menu_shortcut.hpp"
#include <cassert>

int main() {
    using menu_shortcut::Counter;

    // v2.95: one F3 press no longer opens the menu; four quick taps do.
    {
        Counter c;
        bool opened = false;
        for (int i = 1; i <= 3; ++i) {
            int count = c.feed(true, true, false, true, 1000.0 + i * 100.0, opened);
            assert(!opened);
            assert(count == i);
            // the key up is not a new press
            c.feed(true, false, false, true, 1000.0 + i * 100.0 + 40.0, opened);
            assert(!opened);
        }
        c.feed(true, true, false, true, 1400.0, opened);
        assert(opened);
        assert(c.presses == 0);
        // the key up of the tap that opened the menu must not start a new sequence
        c.feed(true, false, false, true, 1420.0, opened);
        assert(!opened && c.presses == 0);
    }

    // A slow tap starts the count over instead of completing it.
    {
        Counter c;
        bool opened = false;
        c.feed(true, true, false, true, 0.0, opened);
        c.feed(true, true, false, true, 100.0, opened);
        c.feed(true, true, false, true, 200.0, opened);
        assert(!opened);
        assert(c.feed(true, true, false, true, 1000.0, opened) == 1);  // gap too long
        assert(!opened);
    }

    // F3 held down (auto-repeat) never opens the menu.
    {
        Counter c;
        bool opened = false;
        for (int i = 0; i < 10; ++i) {
            c.feed(true, true, true, true, i * 30.0, opened);
            assert(!opened);
        }
        assert(c.presses == 0);
    }

    // Nothing opens outside a level, and other keys don't count.
    {
        Counter c;
        bool opened = false;
        for (int i = 0; i < 6; ++i) {
            c.feed(true, true, false, false, i * 50.0, opened);
            assert(!opened);
        }
        for (int i = 0; i < 6; ++i) {
            c.feed(false, true, false, true, i * 50.0, opened);
            assert(!opened);
        }
        assert(c.presses == 0);
    }

    // A non-monotonic clock doesn't count as a quick tap.
    {
        Counter c;
        bool opened = false;
        c.feed(true, true, false, true, 500.0, opened);
        assert(c.feed(true, true, false, true, 100.0, opened) == 1);
        assert(!opened);
    }
}
