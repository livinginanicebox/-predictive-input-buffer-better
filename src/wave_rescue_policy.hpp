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

    // v2.95 tight corridors. The turn-away check used to demand the whole escape window after
    // the turn (16 ticks, 24 inverted). In a corridor narrower than that the turned run clears
    // the obstacle and is simply still inside the corridor, so the old rule called a perfectly
    // reactable crash a death. A turn a couple of ticks before the crash that clears this much
    // room now counts too, which is what makes frame-perfect wave spam through tight gaps work.
    constexpr int tightClearance(bool inverted) {
        return inverted ? 8 : 6;
    }

    constexpr bool tightTurnSurvives(int survived, int turnAt, int clearance) {
        return survived >= turnAt + clearance;
    }

    // v2.97 wave slack. A wave run that only "survives" because a turn a few ticks before the
    // crash was assumed is a run that flies straight at the obstacle and bets on your next click.
    // Picking that timing (the first one that counts as cleared) is what made wave sections
    // harder: one or two frames of human timing error and the wave is inside the spike. A clean
    // clear is a run that survives the look-ahead on its own, with no assumed turn.
    constexpr bool cleanClear(int survived, int target) {
        return survived >= target;
    }

    // How far past the first rescued timing the search keeps looking for a clean one. Small on
    // purpose: the player is waiting for the input, and a clean timing inside a frame of the
    // rescued one is already a real improvement.
    constexpr int cleanSearchSlack(int ticksPerFrame) {
        return std::max(1, ticksPerFrame);
    }

    // One more tick of room around a clean run, so a tick of real-life difference on either
    // side still clears the same obstacle.
    constexpr bool hasSlack(int firstClean, int lastClean) {
        return lastClean > firstClean;
    }

    // v2.97 early release. Holding in wave only reads as "a spike in a few frames" once the held
    // run dies inside the release window, and letting go has to be accepted there even when the
    // release still ends later - waiting for the player's own click is exactly what sent the
    // wave into the spike. `lead` is the wave release window (a couple of frames ahead).
    constexpr int releaseLead(int window, int waveLead) {
        return std::max(window, waveLead);
    }

    constexpr bool releaseWins(bool relaxed, int release, int hold, int horizon, int minGain) {
        if (release >= horizon) {
            return true;
        }
        return relaxed && release >= hold + minGain;
    }
}
