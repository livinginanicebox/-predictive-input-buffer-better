#include "../src/dual_buffer.hpp"
#include <cassert>

// The pure decisions behind dual-mode buffering: which player an input acts on,
// which ghost gets the delayed press, and how the queued rhythm is shifted.
int main() {
    using namespace dual_buffer;

    // v2.55 (unchanged): two-player level, dual mode OFF -> a P2-side input steers Player 1.
    assert(actsOnPlayer1(true, false, false));
    // v2.95: two-player level, dual mode ON -> P2 steers Player 2 (P2 is buffered, predicted on P2).
    assert(!actsOnPlayer1(true, true, false));
    // normal level, dual mode on -> each player steers themselves
    assert(actsOnPlayer1(false, true, true));
    assert(!actsOnPlayer1(false, true, false));
    // dual mode off -> P1 inputs act on P1 (unchanged)
    assert(actsOnPlayer1(false, false, true));
    assert(!actsOnPlayer1(false, false, false));

    // the acting player's ghost is always the primary one; the other ghost never takes this input
    assert(pressGhostIndex(true, true) == 0);
    assert(pressGhostIndex(false, true) == 0);
    assert(pressGhostIndex(true, false) == 0);

    // both ghosts stepped in dual (both players' survival judged), only the primary ghost when solo
    assert(stepOtherGhost(true, true, true));
    assert(!stepOtherGhost(true, false, true));   // no second player: nothing to step
    assert(!stepOtherGhost(true, true, false));   // ghost not ready: never claim a prediction we didn't run
    assert(!stepOtherGhost(false, true, true));   // solo: no second ghost

    // the other player's queued press is followed in the prediction (coordinated duals)
    assert(otherQueuePress(true, true));
    assert(!otherQueuePress(true, false));
    assert(!otherQueuePress(false, true));

    // queued rhythm is shifted like the acting player's (same rule, per player now)
    assert(queuedTicks(10, 0, 10) == 10);
    assert(queuedTicks(10, 1, 9) == 9);   // CBF mid-tick click: fire one tick earlier
    assert(queuedTicks(10, 1, 10) == 10); // never jump earlier than the queued input itself
    assert(queuedTicks(2, 1, 10) == 10);  // never jump earlier than the queued input itself
    return 0;
}
