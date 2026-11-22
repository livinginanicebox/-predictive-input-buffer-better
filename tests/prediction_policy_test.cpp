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
    // A surviving ghost that misses the orb is NOT a successful click.
    assert(!orbActivatedByPress(false, -1, 0, 2));
    assert(!orbActivatedByPress(true, -1, 0, 2));
    assert(!orbActivatedByPress(true, 20, 0, 2)); // held into it much later
    assert(!orbActivatedByPress(false, 0, 0, 2)); // wrong orb
    assert(orbActivatedByPress(true, 0, 0, 2));
    assert(orbActivatedByPress(true, 2, 0, 2));
    assert(orbActivatedByPress(true, 9, 8, 2)); // confirmed buffered activation
    assert(!orbActivatedByPress(true, 7, 8, 2)); // orb fired before this press
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
    assert(allowWaveRecovery(false, false, false));
    assert(!allowWaveRecovery(false, true, false)); // release after blue orb
    assert(!allowWaveRecovery(false, false, true)); // flip during prediction
    assert(!allowWaveRecovery(false, true, true));
    assert(allowWaveRecovery(true, true, true)); // presses keep normal recovery
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
