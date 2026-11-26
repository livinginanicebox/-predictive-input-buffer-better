#include "../src/survival_policy.hpp"
#include <cassert>

using survival_policy::Action;
using survival_policy::choose;
using survival_policy::name;
using survival_policy::wantsDown;

int main() {
    // Nothing to fix: what you are doing lives past the deadline.
    {
        auto c = choose(Action::Hold, 2400, 2400, 900, 1200, 240, 12);
        assert(c.action == Action::Hold);
        assert(!c.takeOver);
    }

    // v2.97.1: even a doom far away is not touched - only an imminent one.
    {
        auto c = choose(Action::Release, 600, 600, 2400, 2400, 240, 12);
        assert(c.action == Action::Release);
        assert(!c.takeOver);
    }

    // Holding dies inside the deadline, letting go lives long -> let go.
    {
        auto c = choose(Action::Hold, 20, 20, 2400, 900, 240, 12);
        assert(c.action == Action::Release);
        assert(c.takeOver);
        assert(c.ticks == 2400);
    }

    // Releasing dies soon, tapping lives longest -> tap.
    {
        auto c = choose(Action::Release, 12, 300, 12, 480, 240, 12);
        assert(c.action == Action::Tap);
        assert(c.takeOver);
    }

    // Inside the margin: not worth a press.
    {
        auto c = choose(Action::Hold, 30, 30, 40, -1, 240, 12);
        assert(c.action == Action::Hold);
        assert(!c.takeOver);
        // ...but one tick past the margin is enough.
        auto d = choose(Action::Hold, 30, 30, 43, -1, 240, 12);
        assert(d.action == Action::Release);
        assert(d.takeOver);
    }

    // A tie keeps what the game is already doing.
    {
        auto c = choose(Action::Release, 300, 300, 300, 300, 240, 0);
        assert(c.action == Action::Release);
        assert(!c.takeOver);
    }

    // Unknown runs (-1) are never picked.
    {
        auto c = choose(Action::Hold, 40, 40, -1, -1, 240, 0);
        assert(c.action == Action::Hold);
        assert(!c.takeOver);
    }

    // The incumbent is never compared against itself.
    {
        auto c = choose(Action::Tap, 100, 50, 50, 100, 240, 0);
        assert(c.action == Action::Tap);
        assert(!c.takeOver);
    }

    // Names for the console line.
    {
        assert(name(Action::Hold)[0] == 'h');
        assert(name(Action::Release)[0] == 'r');
        assert(name(Action::Tap)[0] == 't');
        assert(name(Action::None)[0] == 'n');
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
