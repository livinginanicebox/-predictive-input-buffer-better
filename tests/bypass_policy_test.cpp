#include "../src/bypass_policy.hpp"
#include <cassert>

int main() {
    using bypass_policy::bufferActive;
    using bypass_policy::State;

    // v2.96: holding the bypass key turns an enabled stage off, and the stage comes
    // straight back when the key is released.
    assert(bufferActive(true, false));
    assert(!bufferActive(true, true));
    // A stage that is switched off stays off - holding the key can't switch it on.
    assert(!bufferActive(false, false));
    assert(!bufferActive(false, true));

    // One press -> held; the key-up -> back to the stage.
    {
        State s;
        assert(s.feed(true, true, false));
        assert(s.held);
        assert(!bufferActive(true, s.held));
        assert(s.feed(true, false, false));
        assert(!s.held);
        assert(bufferActive(true, s.held));
    }

    // Auto-repeat while held is not a new press / release.
    {
        State s;
        s.feed(true, true, false);
        for (int i = 0; i < 10; ++i) {
            assert(!s.feed(true, true, true));
            assert(s.held);
        }
        // Other keys are not the bypass key.
        assert(!s.feed(false, false, false));
        assert(s.held);
    }

    // Repeated events with no change report no change.
    {
        State s;
        assert(!s.feed(true, false, false));  // release first, nothing held
        assert(!s.held);
        s.feed(true, true, false);
        assert(!s.feed(true, true, false));
        assert(s.held);
    }

    // The poll ends a bypass whose key-up was lost (alt-tab) and starts nothing on its own.
    {
        State s;
        s.feed(true, true, false);
        assert(s.poll(false));   // the key-up that never reached us
        assert(!s.held);
        assert(!s.poll(false));  // nothing left to change
        // The caller only polls while a bypass is on, but the poll itself follows the
        // OS state either way.
        assert(s.poll(true));
        assert(s.held);
    }

    // A level start / restart clears the bypass.
    {
        State s;
        s.feed(true, true, false);
        s.reset();
        assert(!s.held);
        assert(bufferActive(true, s.held));
    }
}
