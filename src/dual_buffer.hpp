#pragma once

#include <algorithm>

// Dual mode (two-player levels, or dual mode in any level): the two players run the level
// at the same time and may act independently. Buffering each player's inputs separately
// is what makes asymmetrical duals (mixed-form sections where only one side presses)
// practical: P1's click must never be judged or delayed for P2, and P2's click must never
// be judged or delayed for P1.
//
// This module holds the pure decisions: no ghosts, no PlayLayer, no queue.
namespace dual_buffer {
    // Which real player an input acts on.
    //
    // v2.55: in a two-player level outside dual mode, a P2-side input steers Player 1
    // (Relief's wave parts), so it is buffered as a P1 input.
    // v2.95: everywhere else, both players steer themselves and every input is buffered
    // on the player whose button was pressed.
    constexpr bool actsOnPlayer1(bool twoPlayer, bool dualMode, bool isPlayer1) {
        return twoPlayer && !dualMode && !isPlayer1 ? true : isPlayer1;
    }

    // The delayed press is only ever delivered to the ghost whose player pressed.
    // In dual mode the ghost for the other player keeps running on its own state.
    // (0 = the acting ghost is always the first / primary ghost, 1 = the other.)
    constexpr int pressGhostIndex(bool actsOnPlayer1, bool dualMode) {
        (void)actsOnPlayer1;  // the acting player's ghost is always ghost 0
        (void)dualMode;
        return 0;
    }

    // The other player should be stepped (kept in sync) in the prediction when it exists
    // and dual mode is on. Both players' survival is judged: a dead second player is a
    // failed level just as much as a dead first one.
    constexpr bool stepOtherGhost(bool dualMode, bool otherPlayerPresent, bool ghost2Ready) {
        return dualMode && otherPlayerPresent && ghost2Ready;
    }

    // A queued press for the other player (in dual mode) is followed, at the tick the
    // press would really fire in the real run, so a coordinated dual press is judged as
    // a coordinated dual press. A held, un-pressed other ghost under-models survival.
    constexpr bool otherQueuePress(bool dualMode, bool otherQueued) {
        return dualMode && otherQueued;
    }

    // Delay of a queued input: the same delay shift as for the acting player, so a
    // coordinated press in a symmetric dual keeps its shared rhythm.
    constexpr int queuedTicks(int shift, int early, int after) {
        return std::max(shift - early, after);
    }
}
