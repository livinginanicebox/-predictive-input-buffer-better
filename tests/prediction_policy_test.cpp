#include "../src/prediction_policy.hpp"
#include <cassert>
#include <cmath>

struct Player {
    bool m_isDart;
    bool m_isUpsideDown;
};

int main2();

int main() {
    using namespace prediction_policy;
    assert(passOrbPress(true, true, true, false));
    assert(!passOrbPress(false, true, true, false));
    assert(!passOrbPress(true, false, true, false));
    assert(!passOrbPress(true, true, false, false)); // keep pending inputs ordered
    // v2.86 "orb timing": a press inside an orb is re-timed instead of passed through.
    assert(!passOrbPress(true, true, true, true));
    int orb1 = 1, orb2 = 2;
    assert(sameOrb(&orb1, &orb1));
    assert(!sameOrb(&orb1, &orb2));
    assert(!sameOrb(static_cast<int*>(nullptr), static_cast<int*>(nullptr)));
    assert(betterOrbFallback(true, true, 5, -1)); // unsafe can still activate the intended orb
    assert(!betterOrbFallback(true, false, 100, -1)); // never switch targets
    assert(!betterOrbFallback(false, true, 5, -1));
    assert(!betterOrbFallback(true, true, 5, 5));

    Player uprightWave{true, false};
    Player invertedWavePlayer{true, true};
    Player invertedCube{false, true};
    assert(!invertedWave(&uprightWave, &invertedWavePlayer, false));
    assert(invertedWave(&uprightWave, &invertedWavePlayer, true));
    assert(invertedWave(&invertedWavePlayer, &uprightWave, false));
    assert(!invertedWave(&invertedCube, &uprightWave, true));
    assert(!invertedWave(static_cast<Player*>(nullptr), &uprightWave, true));

    assert(waveSpamGap(16, false, 6) == 16);
    assert(waveSpamGap(16, true, 6) == 6);
    assert(waveSpamGap(4, true, 6) == 4); // respect a smaller user setting
    assert(waveMinimumTurn(false, 6, 12) == 6);
    assert(waveMinimumTurn(true, 6, 12) == 12);
    assert(waveEscape(false, 16, 24) == 16);
    assert(waveEscape(true, 16, 24) == 24);
    assert(!strictWaveRecovery(false, false));
    assert(strictWaveRecovery(true, false));
    // v2.95: a gravity portal in the middle of a prediction no longer makes the whole run
    // "strict" (that is what made upside-down <-> normal transitions the hardest part to
    // buffer); only starting inverted keeps the small extra margin.
    assert(!strictWaveRecovery(false, true));
    assert(strictWaveRecovery(true, true));
    return main2();
}

// v2.94: ground orb intent, ship spam exemption, wave spam streak, ship straight band.
int main2() {
    using namespace prediction_policy;
    assert(groundOrbIntent(3, GROUND_ORB_REACH));
    assert(!groundOrbIntent(5, GROUND_ORB_REACH));
    assert(!groundOrbIntent(-1, GROUND_ORB_REACH));
    assert(spamStreak(true, 3) == 5);
    assert(spamStreak(false, 3) == 3);
    assert(leaveSafeInput(true, true));
    assert(!leaveSafeInput(true, false));
    assert(!leaveSafeInput(false, true));
    // v2.94: the old 48-unit band retimed ordinary feather recovery as "wobbly".
    assert(straightBand(true) < straightBand(false));
    assert(straightBand(true) >= 60.f);
    assert(straightBand(false) >= 90.f);

    // v2.95 frame counter: ticks -> frames from the real step length, with decimals that fit.
    // 240 steps/s (60 fps): one tick is a quarter frame, 33 ticks are 8.25 frames.
    assert(std::abs(framesFromTicks(1, 1.0 / 240.0) - 0.25) < 1e-9);
    assert(std::abs(framesFromTicks(8, 1.0 / 240.0) - 2.0) < 1e-9);
    assert(std::abs(framesFromTicks(33, 1.0 / 240.0) - 8.25) < 1e-9);
    // Unlocked FPS: 1/288 s steps give 8.33 frames for 40 ticks.
    assert(std::abs(framesFromTicks(40, 1.0 / 288.0) - 8.3333) < 1e-3);
    assert(frameDecimals(0.25) == 2);   // 0.25f
    assert(frameDecimals(1.9) == 2);    // 1.9f
    assert(frameDecimals(8.33) == 2);   // 8.33f
    assert(frameDecimals(12.5) == 1);   // 12.5f
    assert(frameDecimals(240.0) == 0);  // 240f
    assert(framesFromTicks(4, 0.0) > 0.9);  // a bad step length falls back to 240/s

    // v2.95 orbs: a press inside an orb is never pushed behind a waiting delay.
    assert(orbPressOvertakesQueue(true, true, false));
    assert(!orbPressOvertakesQueue(true, true, true));
    assert(!orbPressOvertakesQueue(false, true, false));
    assert(!orbPressOvertakesQueue(true, false, false));
    // ... and a press in front of an orb is that orb's click.
    assert(orbFallbackWanted(true, true, ORB_FALLBACK_MIN_LIFE, ORB_FALLBACK_MIN_LIFE));
    assert(!orbFallbackWanted(true, true, ORB_FALLBACK_MIN_LIFE - 1, ORB_FALLBACK_MIN_LIFE));
    assert(!orbFallbackWanted(true, false, 200, ORB_FALLBACK_MIN_LIFE));
    assert(!orbFallbackWanted(false, true, 200, ORB_FALLBACK_MIN_LIFE));
    // A dying click is never "the best click".
    assert(orbClickBeatsAll(true, false));
    assert(orbClickBeatsAll(false, true));
    assert(!orbClickBeatsAll(false, false));

    // v2.95 ship: only full clears or short taps whose crash moves further out.
    int horizon = 80;
    assert(shipDelayAccepted(false, 2, 20, 30, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN));  // 2+30 >= 20+12
    assert(!shipDelayAccepted(false, 2, 40, 30, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN)); // 2+30 < 40+12
    assert(!shipDelayAccepted(true, 2, 20, 30, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN));  // straight-fly: no marginal
    assert(!shipDelayAccepted(false, SHIP_MAX_TAP_DELAY + 1, 20, 30, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN)); // long delay
    assert(shipDelayAccepted(false, 40, 20, 30, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN) == false);
    assert(shipDelayAccepted(false, 40, 20, horizon, horizon, SHIP_MAX_TAP_DELAY, SHIP_CRASH_MARGIN)); // full clear, any delay
    assert(SHIP_MAX_TAP_DELAY <= 12);  // at most a few frames, never a big retime
    return 0;
}
