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

    // v2.95: an orb the press is already touching belongs to the player, on the ground too,
    // and a timing inside the orb that lives clearly past a dying click is a save.
    assert(passOrbPress(true, true, true, false));
    // ...but a ground press standing in an orb does not: it is a jump, so it gets predicted
    // (passOrbPress is called with the touched-orb flag cleared for it).
    assert(!passOrbPress(true, false, true, false));
    assert(pressOnOrb(0, true));
    assert(!pressOnOrb(0, false));   // a queued input must keep its order
    assert(!pressOnOrb(3, true));
    assert(orbLateSave(10, 60, 200, 0, 20));      // late line off: just must live longer
    assert(!orbLateSave(10, 25, 200, 0, 20));     // not clearly longer than the click
    assert(orbLateSave(10, 95, 200, 90, 20));     // past the late-crash line
    assert(!orbLateSave(10, 80, 200, 90, 20));    // dies at the orb itself
    assert(!orbLateSave(200, 300, 200, 0, 20));   // the click already cleared: nothing to save
    // full clear is always usable; a dying click + a clearly later tick is the v2.95 fix
    assert(orbPickUsable(true, false, true, false, true, true, false));
    assert(orbPickUsable(true, false, false, true, true, true, false));   // late save, cube too
    assert(orbPickUsable(true, false, false, true, false, true, false));  // late save, other modes
    assert(!orbPickUsable(false, false, false, true, false, true, false)); // never a non-orb tick
    assert(!orbPickUsable(true, false, false, false, true, true, true));   // needs full: no
    assert(!orbPickUsable(true, false, false, false, false, false, true)); // working click stays
    assert(orbPickUsable(true, false, false, false, false, true, true));   // dying click, lives as long
    // v2.92: a re-time only needs to live longer, whatever the mode needs for a fresh press
    assert(orbPickUsable(true, true, false, false, false, true, true));
    assert(!orbPickUsable(true, true, false, false, false, true, false));
    assert(orbPickUsable(true, true, false, true, false, true, false));    // late line saves it
    assert(orbChainLookahead(142, 0, 24, 260) == 142);
    assert(orbChainLookahead(142, 1, 24, 260) == 166);
    assert(orbChainLookahead(142, 4, 24, 260) == 238);
    assert(orbChainLookahead(240, 4, 24, 260) == 260);  // capped for big stages
    return 0;
}
