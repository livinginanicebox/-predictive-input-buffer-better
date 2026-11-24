#pragma once

#include <algorithm>
#include <unordered_set>

namespace wave_rescue_policy {
    template <typename Object>
    class GravityPortalTracker {
        std::unordered_set<Object*> m_used1, m_used2;
    public:
        bool activate(Object* portal, bool second) {
            return portal && (second ? m_used2 : m_used1).insert(portal).second;
        }
        void clear() {
            m_used1.clear();
            m_used2.clear();
        }
    };

    // Only an opposite input after exactly one queued input can be represented by
    // the two-event simulation. Robot jumps are rescued on release only.
    constexpr bool canRescue(int queued, bool firstDown, bool nextDown, bool wave, bool ship, bool robot) {
        return queued == 1 && firstDown != nextDown &&
            (wave || ship || (robot && firstDown && !nextDown));
    }

    constexpr bool preserveQueueOnSpam(bool wave, bool oldWaveRules, bool queueEmpty) {
        return wave && !oldWaveRules && !queueEmpty;
    }

    constexpr int lookahead(int baseHorizon, int interval, int clearance) {
        return std::max(baseHorizon, interval + clearance);
    }

    constexpr bool portalIsSafe(bool rejectPortalSafety, int tick, int delay) {
        return !rejectPortalSafety && tick >= delay;
    }

    constexpr bool diedBeforeNextTurn(int survived, int interval) {
        return survived < interval;
    }

    constexpr bool clearsNextTurn(bool turnFired, int survived, int interval, int clearance) {
        return turnFired && survived >= interval + clearance;
    }

    // v2.95 "wave precision": on hard wave levels (Slaughterhouse, Tidal Wave)
    // the ghost used to stop looking four frames before the next corridor, and a
    // turn that only survived because you could still react was called safe on the
    // spot. Look four more frames out, check a turn two ticks after the input, and
    // demand a longer clearance before calling a crash recoverable.
    constexpr int PRECISION_TAIL = 16;
    constexpr int PRECISION_MIN_TURN = 2;
    constexpr int PRECISION_CLEARANCE_BONUS = 8;
    constexpr int PRECISION_CENTER_WINDOW = 6;
    constexpr int PRECISE_TURN_MARGINS[] = { 2, 4, 8, 12, 16 };
    constexpr int PLAIN_TURN_MARGINS[] = { 4, 8, 12 };

    inline int turnCount(bool precision) {
        return precision ? static_cast<int>(sizeof(PRECISE_TURN_MARGINS) / sizeof(PRECISE_TURN_MARGINS[0]))
                         : static_cast<int>(sizeof(PLAIN_TURN_MARGINS) / sizeof(PLAIN_TURN_MARGINS[0]));
    }

    inline int turnMargin(bool precision, int index) {
        return precision ? PRECISE_TURN_MARGINS[index] : PLAIN_TURN_MARGINS[index];
    }

    // Extra ticks of look-ahead over the user's wave setting (both already in
    // physics steps; the caller does the 240 Hz -> current FPS conversion).
    constexpr int tail(bool precision, int baseTail) {
        return baseTail + (precision ? PRECISION_TAIL : 0);
    }

    inline int recoveryClearance(bool precision, bool strict, int normal, int inverted) {
        int value = strict ? inverted : normal;
        return value + (precision ? PRECISION_CLEARANCE_BONUS : 0);
    }

    // A turn that is only safe because the player must react is not a save worth
    // passing through untouched on a dense level: try a small delay first that
    // clears the look-ahead on its own. Low latency / training mode opt out.
    constexpr bool centerSafeTurn(bool precision, bool usedRecovery, bool lowLatency, bool training) {
        return precision && usedRecovery && !lowLatency && !training;
    }
}
