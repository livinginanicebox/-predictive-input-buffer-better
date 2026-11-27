#pragma once

#include <algorithm>

// Policy values are in physics ticks; callers convert 240 Hz constants with st().
namespace prediction_policy {
    // Never reorder a pending input sequence. Otherwise let an intentional press on
    // an orb reach the game unchanged, regardless of the predicted outcome - unless
    // "orb timing" is on and the press may be re-timed inside that same orb.
    constexpr bool passOrbPress(bool down, bool touchingOrb, bool queueEmpty, bool retimeInOrb) {
        return down && touchingOrb && queueEmpty && !retimeInOrb;
    }

    template <typename T>
    constexpr bool sameOrb(T const* target, T const* used) {
        return target && target == used;
    }

    constexpr bool betterOrbFallback(bool enabled, bool sameTarget, int survival, int bestSurvival) {
        return enabled && sameTarget && survival > bestSurvival;
    }

    template <typename Player>
    bool invertedWave(Player const* p1, Player const* p2, bool dual) {
        return (p1 && p1->m_isDart && p1->m_isUpsideDown) ||
            (dual && p2 && p2->m_isDart && p2->m_isUpsideDown);
    }

    inline int waveSpamGap(int configuredGap, bool invertedWave, int invertedGap) {
        return invertedWave ? std::min(configuredGap, invertedGap) : configuredGap;
    }

    constexpr bool strictWaveRecovery(bool startedInverted, bool gravityChanged) {
        return startedInverted || gravityChanged;
    }

    inline int waveMinimumTurn(bool invertedWave, int normalMinimum, int invertedMinimum) {
        return invertedWave ? invertedMinimum : normalMinimum;
    }

    inline int waveEscape(bool invertedWave, int normalEscape, int invertedEscape) {
        return invertedWave ? invertedEscape : normalEscape;
    }

    // v2.94: on the ground a press is a jump, so a jump that works is only turned into an orb
    // click when the orb is this close (4 ticks = 1 frame) - there the press is already on the
    // orb in the real run and the jump is what misses it.
    constexpr int GROUND_ORB_REACH = 4;

    constexpr bool groundOrbIntent(int touchTick, int reach) {
        return touchTick >= 0 && touchTick <= reach;
    }

    // v2.94: wave gets a longer spam streak than the other modes (5 instead of 3): Tidal Wave
    // style spam was going straight to the game after 3 fast clicks, so the buffer stopped
    // helping exactly where the level is hardest.
    constexpr int spamStreak(bool wave, int base) {
        return wave ? base + 2 : base;
    }

    // v2.94: a ship input that already survives the whole look-ahead is left exactly where it
    // is. The half-tick "safe only at the exact spot" re-search is a cube / orb rule; applying
    // it to ship retimed straight-fly feathers that were fine and pushed the ship off its line.
    constexpr bool leaveSafeInput(bool ship, bool survivesLookahead) {
        return ship && survivesLookahead;
    }

    // v2.94: straight-fly tolerance. A recovered ship run that stays inside this vertical band
    // counts as level enough; the old 48 units (1.6 blocks) called normal feather recovery
    // "wobbly" and retimed taps that did not need it.
    inline float straightBand(bool mini) {
        return mini ? 60.f : 90.f;
    }

    // v2.95: an orb the press is already touching always belongs to the player, even on the
    // ground. v2.94 only reached this path for airborne presses ("ground jump is safe" returns
    // before the orb scan), so clicking an orb from a platform while the jump looked fine let
    // the raw click jump into the obstacle instead of firing the orb.
    constexpr bool pressOnOrb(int touchTick, bool queueEmpty) {
        return queueEmpty && touchTick == 0;
    }

    // v2.95: a timing inside the orb that does not clear the whole look-ahead is still a save
    // when the click you made is itself a death and this timing lives clearly longer - the
    // "nothing beats my click" pass-through that let an early orb click kill you. It must buy
    // at least ORB_RETIME_MARGIN over your click and reach the late-crash line (unless that
    // line is off), the same best-effort rule the main search uses.
    constexpr bool orbLateSave(int now, int ret, int horizon, int lateLine, int margin) {
        return now < horizon && ret > now + margin && (lateLine <= 0 || ret >= lateLine);
    }

    // v2.95: which tick inside the orb may be used instead of the click you made.
    //   full      - it clears the whole (longer) orb look-ahead
    //   late      - best effort: it lives clearly past your click and past the late-crash line,
    //               so the crash that is left belongs to the obstacle behind the orb
    //   retiming  - the click already used this orb and is being re-timed inside it
    //   needsFull - the tick is in the opportunistic deep part of the scan, or the mode requires
    //               a fully safe orb path (cube / robot / ship)
    //   dying     - the click you made is itself a death
    //   livesLong - it survives at least as long as that click (strictly longer for a re-time)
    // The rejection that mattered: a tick that lives clearly longer than a click that is itself a
    // death used to be thrown away unless it cleared everything, so the raw click - the one the
    // prediction already knows kills you - was passed straight to the game and the orb never
    // fired. That is the "nothing beats my click" report.
    constexpr bool orbPickUsable(bool usesOrb, bool retiming, bool full, bool late, bool needsFull, bool dying, bool livesLong) {
        if (!usesOrb) {
            return false;
        }
        if (full || late) {
            return true;
        }
        if (retiming) {
            return dying && livesLong;
        }
        if (needsFull || !dying) {
            return false;
        }
        return livesLong;
    }

    // v2.95: how much further than the orb pass the ghost looks for an orb chain. A press that
    // is early for a chain of two or three orbs needs the crash behind the last one to count as
    // "safe"; the old setting-sized look-ahead stopped before it, the orb path read safe and the
    // click was passed through unsaved.
    inline int orbChainLookahead(int horizon, int chain, int perOrb, int cap) {
        return chain > 0 ? std::min(horizon + chain * perOrb, cap) : horizon;
    }

}
