#include "../src/survival_policy.hpp"
#include <cassert>

using survival_policy::Action;
using survival_policy::choose;
using survival_policy::wantsDown;

int main() {
    // Nothing to fix: what you are doing covers the whole horizon.
    {
        auto c = choose(Action::Hold, 2400, 2400, 900, 1200, true);
        assert(c.action == Action::Hold);
        assert(!c.takeOver);
    }

    // Holding dies fast, letting go lives long -> let go.
    {
        auto c = choose(Action::Hold, 20, 20, 2400, 900, false);
        assert(c.action == Action::Release);
        assert(c.takeOver);
        assert(c.ticks == 2400);
    }

    // Releasing dies fast, tapping lives longest -> tap.
    {
        auto c = choose(Action::Release, 12, 300, 12, 480, false);
        assert(c.action == Action::Tap);
        assert(c.takeOver);
    }

    // A tie keeps what the game is already doing.
    {
        auto c = choose(Action::Release, 300, 300, 300, 300, false);
        assert(c.action == Action::Release);
        assert(!c.takeOver);
    }

    // Unknown runs (-1) are never picked.
    {
        auto c = choose(Action::Hold, 40, 40, -1, -1, false);
        assert(c.action == Action::Hold);
        assert(!c.takeOver);
    }

    // The incumbent is never compared against itself.
    {
        auto c = choose(Action::Tap, 100, 50, 50, 100, false);
        assert(c.action == Action::Tap);
        assert(!c.takeOver);
    }

    // Strictly better wins even by one tick.
    {
        auto c = choose(Action::Hold, 41, 41, 42, -1, false);
        assert(c.action == Action::Release);
        assert(c.takeOver);
    }

    // Button states: hold is always down, release never, tap alternates every `tapTicks`.
    {
        assert(wantsDown(Action::Hold, 0, 12));
        assert(wantsDown(Action::Hold, 999, 12));
        assert(!wantsDown(Action::Release, 0, 12));
        assert(!wantsDown(Action::Release, 999, 12));
        assert(wantsDown(Action::Tap, 0, 12));
        assert(wantsDown(Action::Tap, 11, 12));
        assert(!wantsDown(Action::Tap, 12, 12));
        assert(!wantsDown(Action::Tap, 23, 12));
        assert(wantsDown(Action::Tap, 24, 12));
        assert(!wantsDown(Action::None, 0, 12));
        // A zero tap length can't divide by zero.
        assert(wantsDown(Action::Tap, 0, 0));
    }
}
