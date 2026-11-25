#include "../src/menu_shortcut.hpp"
#include "../src/bypass_policy.hpp"
#include <cassert>

// v2.95: the two new shortcuts, exercised together the way the game drives them.
int main() {
    using menu_shortcut::Counter;

    // Four quick F3 taps open the menu; the bypass key is independent of that.
    {
        Counter taps;
        bypass_policy::State bypass;
        bool opened = false;
        double t = 0.0;
        for (int i = 0; i < 4; ++i) {
            t += 120.0;
            taps.feed(true, true, false, true, t, opened);
            bool thisTapOpened = opened;  // the key-up event resets the out-param
            taps.feed(true, false, false, true, t + 50.0, opened);
            opened = thisTapOpened;
            // holding the bypass key while tapping F3 is fine, it doesn't feed the counter
            // (only the first key-down changes the state; repeats are not new presses)
            bool changed = bypass.feed(true, true, false);
            assert(changed == (i == 0));
            assert(!bypass.poll(true));
            assert(bypass.held);
        }
        assert(opened);
        assert(bypass.held);
        // opening the menu releases a held bypass (the poll sees the key as up)
        assert(bypass.poll(false));
        assert(!bypass.held);
        assert(bypass_policy::bufferActive(true, bypass.held));
    }

    // A bypass that is held across a level load is dropped by the game reset path
    // (dropGhosts() clears buf.bypassed), so a fresh State models the next level.
    {
        bypass_policy::State s;
        s.feed(true, true, false);
        assert(!bypass_policy::bufferActive(true, s.held));
        bypass_policy::State fresh;
        assert(bypass_policy::bufferActive(true, fresh.held));
    }
}
