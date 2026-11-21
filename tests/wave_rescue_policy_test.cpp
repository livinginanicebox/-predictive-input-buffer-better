#include "../src/wave_rescue_policy.hpp"
#include <cassert>

int main() {
    using namespace wave_rescue_policy;
    // Both wave directions must be checked across normal/inverted transitions.
    assert(canRescue(1, true, false, true, false, false));
    assert(canRescue(1, false, true, true, false, false));
    assert(!canRescue(1, false, false, true, false, false));
    assert(!canRescue(2, false, true, true, false, false));
    assert(canRescue(1, true, false, false, false, true));
    assert(!canRescue(1, false, true, false, false, true));
    assert(canRescue(1, false, true, false, true, false));
    assert(!canRescue(1, false, true, false, false, false));

    assert(preserveQueueOnSpam(true, false, false));
    assert(!preserveQueueOnSpam(true, false, true));
    assert(!preserveQueueOnSpam(true, true, false));
    assert(!preserveQueueOnSpam(false, false, false));

    // A lookahead ending before or right after the second turn is not a save.
    assert(lookahead(50, 55, 20) == 75);
    assert(lookahead(80, 30, 20) == 80);
    assert(portalIsSafe(false, 5, 5));
    assert(!portalIsSafe(false, 4, 5));
    assert(!portalIsSafe(true, 20, 5)); // unknown portal cannot prove clearance
    assert(diedBeforeNextTurn(54, 55));
    assert(!diedBeforeNextTurn(55, 55)); // death on the turn tick can still be fixed by delaying it
    assert(!clearsNextTurn(true, 55, 55, 20));
    assert(!clearsNextTurn(true, 74, 55, 20));
    assert(clearsNextTurn(true, 75, 55, 20));
    assert(!clearsNextTurn(false, 75, 55, 20)); // delayed dash release never fired

    // v2.95 tight corridors: a turn that clears a short stretch of room counts, and inverted
    // wave only needs a little more room than upright.
    assert(tightTurnSurvives(26, 20, 6));
    assert(!tightTurnSurvives(25, 20, 6));
    assert(tightClearance(false) == 6);
    assert(tightClearance(true) > tightClearance(false));
    assert(tightTurnSurvives(tightClearance(true) + 20, 20, tightClearance(true)));

    // v2.97 wave slack: a clean clear is a run that survives the look-ahead on its own, so the
    // search prefers it over a timing that only lives because a turn was assumed a few ticks
    // before the crash. The clean search gets one frame of extra ticks, and a clean run with a
    // tick of room on both sides is preferred.
    assert(cleanClear(65, 65));
    assert(!cleanClear(64, 65));
    assert(cleanSearchSlack(4) == 4);
    assert(cleanSearchSlack(0) == 1);
    assert(hasSlack(10, 12));
    assert(!hasSlack(10, 10));

    // v2.97 early release: the wave release window is a couple of frames, and letting go is
    // accepted when it clears the look-ahead or lives clearly longer than holding.
    assert(releaseLead(6, 8) == 8);
    assert(releaseLead(12, 8) == 12);
    assert(releaseWins(true, 40, 30, 65, 4));   // lives 10t longer than holding: release
    assert(!releaseWins(false, 40, 30, 65, 4)); // old strict rule: still dies, keep holding
    assert(releaseWins(true, 65, 30, 65, 4));   // clears the look-ahead either way
    assert(!releaseWins(true, 33, 30, 65, 4));  // barely better: not worth it
    assert(!releaseWins(true, 78, 80, 100, 4)); // a release that dies sooner than holding

    GravityPortalTracker<int> portals;
    int portal = 1, other = 2;
    assert(portals.activate(&portal, false));
    assert(!portals.activate(&portal, false));
    assert(portals.activate(&portal, true)); // dual ghosts are independent
    assert(!portals.activate(&portal, true));
    assert(portals.activate(&other, false));
    assert(!portals.activate(nullptr, false));
    portals.clear();
    assert(portals.activate(&portal, false));
    assert(portals.activate(&portal, true));
}
