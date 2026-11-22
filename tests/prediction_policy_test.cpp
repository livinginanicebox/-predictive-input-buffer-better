#include "../src/prediction_policy.hpp"
#include <cassert>

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
    // v2.95: orb timing alone gates re-timing (no separate alignment toggle). With it
    // off nothing is ever re-timed; with it on an inside-orb press is, unless it is a
    // grounded press without the from-the-ground option, a wave press, or inputs are queued.
    assert(retimeOrbPress(true, true, true, true, true, false, false, false));
    assert(!retimeOrbPress(true, true, true, true, false, false, false, false)); // timing off: never
    assert(!retimeOrbPress(true, true, true, false, true, false, false, false)); // needs prediction
    assert(!retimeOrbPress(false, true, true, true, true, false, false, false)); // releases aren't timed
    assert(!retimeOrbPress(true, false, true, true, true, false, false, false)); // not inside an orb
    assert(!retimeOrbPress(true, true, false, true, true, false, false, false)); // keep pending inputs ordered
    assert(!retimeOrbPress(true, true, true, true, true, true, false, false));  // wave is never orb-timed
    assert(!retimeOrbPress(true, true, true, true, true, false, true, false));  // ground press, from-ground off
    assert(retimeOrbPress(true, true, true, true, true, false, true, true));    // ground press, from-ground on
    // v2.96: an orb the player clicked is never skipped. Airborne early presses are
    // always preserved onto their orb; ground presses only when the jump dies or the
    // orb is already on you; and the longest-living orb activation is used even when
    // it dies sooner than missing the orb would (that death is the next input's
    // problem) - except for ship, which keeps the stricter old rule.
    assert(preserveOrbPress(true, false, false, false));
    assert(!preserveOrbPress(false, true, false, false)); // working ground jump stays a jump
    assert(preserveOrbPress(false, true, true, false));   // ... unless the jump dies
    assert(preserveOrbPress(false, true, false, true));   // ... or the orb is already on you
    assert(useOrbFallback(false, 130, 130, 40, 90));      // dies sooner than missing it: still clicked
    assert(useOrbFallback(false, 50, 130, 10, 90));       // press now dies anyway
    assert(!useOrbFallback(true, 130, 130, 40, 90));      // ship feather that works is left alone
    assert(useOrbFallback(true, 50, 130, 10, 90));        // ship press that dies gets the orb
    assert(useOrbFallback(true, 130, 130, 95, 90));       // ship orb path past the late-crash line
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
    assert(strictWaveRecovery(false, true));
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
    return 0;
}
