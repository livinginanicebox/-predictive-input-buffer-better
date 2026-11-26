#pragma once

// v2.97 survival scan: at any moment there are three scripted things you can do with the
// button - keep holding, stay released, or tap on a fixed rhythm (hold N, let go N, repeat).
// The scan runs each one on the ghost and keeps the one that lives longest.
//
// v2.97.1: it only steps in when what you are doing dies *soon* (within `deadline` ticks) and
// something else lives clearly longer (at least `margin` ticks), so it never presses or lets go
// while you are fine.
namespace survival_policy {
    enum class Action { None, Hold, Release, Tap };

    constexpr char const* name(Action action) {
        switch (action) {
            case Action::Hold: return "hold";
            case Action::Release: return "release";
            case Action::Tap: return "tap";
            default: return "none";
        }
    }

    struct Choice {
        Action action = Action::None;  // what to drive next (None = leave the input alone)
        int ticks = 0;                 // how long that action lived
        bool takeOver = false;         // false = what you are already doing is good enough
    };

    // `currentTicks` is how long the action the game is doing right now lived, `*Ticks` are the
    // others (-1 = not run). Ties, or anything inside the margin, keep what the game is already
    // doing, so an equal-looking scan never causes a pointless press or release.
    inline Choice choose(Action current, int currentTicks, int holdTicks, int releaseTicks, int tapTicks, int deadline, int margin) {
        Choice c;
        c.action = current;
        c.ticks = currentTicks;
        if (currentTicks < 0 || currentTicks > deadline) {
            return c;  // what you are doing lives past the deadline: nothing to fix
        }
        Action order[] = { Action::Hold, Action::Release, Action::Tap };
        int values[] = { holdTicks, releaseTicks, tapTicks };
        for (int i = 0; i < 3; ++i) {
            if (order[i] == current || values[i] < 0) {
                continue;
            }
            if (values[i] > c.ticks + margin) {
                c.action = order[i];
                c.ticks = values[i];
                c.takeOver = true;
            }
        }
        return c;
    }

    // The button state a scripted action wants after `tick` ticks of it.
    inline bool wantsDown(Action action, int tick, int tapTicks) {
        switch (action) {
            case Action::Hold: return true;
            case Action::Release: return false;
            case Action::Tap: {
                int half = tapTicks > 0 ? tapTicks : 1;
                return ((tick / half) % 2) == 0;
            }
            default: return false;
        }
    }
}
