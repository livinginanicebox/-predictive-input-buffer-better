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

    // v2.96: which presses count as "clicked for that orb" and must never be delivered
    // where they would miss it. An early airborne press in front of an orb is that
    // orb's click; a ground press only is when the jump it would make dies, or when
    // the orb is already on you (a working ground jump stays a jump).
    constexpr bool preserveOrbPress(bool preferOrb, bool groundPress, bool pressNowDies, bool nearOrb) {
        return preferOrb || (groundPress && (pressNowDies || nearOrb));
    }

    // v2.96: when no tick inside the orb is fully safe, the longest-living tick that
    // still activates the orb is used anyway - delivering the press anywhere else
    // (passed early, or delayed by the generic search) just skips the orb the player
    // clicked. Ship keeps the old stricter rule: a feather that already works is
    // never held back onto an orb whose path dies sooner, or its line is ruined.
    constexpr bool useOrbFallback(bool ship, int pressNowSurvival, int horizon, int fallbackSurvival, int lateCrashTicks) {
        return !ship || pressNowSurvival < horizon || fallbackSurvival >= lateCrashTicks || fallbackSurvival >= pressNowSurvival;
    }

    // v2.95: orb timing is the only orb gate (the separate "orb alignment" toggle is
    // gone). When it is on it does both jobs the two toggles used to split: an early
    // press is held until it lands inside the orb, and a press already inside an orb
    // is re-timed within that same orb - exactly as if alignment AND timing were on.
    // This decides the inside-an-orb half; grounded presses only re-time when the
    // "also from the ground" timing option is on (a ground press is a jump first).
    constexpr bool retimeOrbPress(bool down, bool touchingOrb, bool queueEmpty, bool smart, bool orbTiming, bool dart,
        bool grounded, bool timingFromGround) {
        return down && touchingOrb && queueEmpty && smart && orbTiming && !dart && (grounded ? timingFromGround : true);
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

}
