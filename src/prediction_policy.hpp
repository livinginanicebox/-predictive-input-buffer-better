#pragma once

#include <algorithm>
#include <cmath>

// Policy values are in physics ticks; callers convert 240 Hz constants with st().
namespace prediction_policy {
    // v2.95 frame counter. The console used to print frames as ticks * 0.25, i.e. always a
    // multiple of a quarter frame, so a save of one or two ticks read as "0f" or as the same
    // number as its neighbour. Frames are now derived from the real physics step length (so
    // unlocked FPS gives values like 1.9 or 8.33 too) and printed with as many decimals as the
    // number needs: two below 10 frames, one below 100, none above.
    inline int frameDecimals(double frames) {
        double magnitude = std::abs(frames);
        if (magnitude < 10.0) {
            return 2;
        }
        return magnitude < 100.0 ? 1 : 0;
    }

    inline double framesFromTicks(double ticks, double stepSeconds) {
        if (!(stepSeconds > 0.0001)) {
            stepSeconds = 1.0 / 240.0;
        }
        return ticks * stepSeconds * 60.0;
    }
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

    // v2.95: an orb click is never pushed behind a pending delay. The press used to be shifted
    // by whatever delay was already waiting, which put the click outside the orb - the orb never
    // fired and it looked like the mod skipped the orb you clicked.
    constexpr bool orbPressOvertakesQueue(bool down, bool touchingOrb, bool queueEmpty) {
        return down && touchingOrb && !queueEmpty;
    }

    // v2.95: a press in front of an orb is that orb's click, even when pressing now "survives"
    // because the orb sits in open air (nothing above or below it). Missing the orb is not what
    // the player asked for, so the press is moved onto the orb as long as the click really fires
    // it and does not die on the spot.
    constexpr int ORB_FALLBACK_MIN_LIFE = 4;  // 1 frame at 60 fps

    constexpr bool orbFallbackWanted(bool preferOrb, bool usesOrb, int orbSurvival, int minSurvival) {
        return preferOrb && usesOrb && orbSurvival >= minSurvival;
    }

    // v2.95: your orb click is only "the best click" when it survives the look-ahead. A click
    // that dies inside it, with nothing inside the orb living longer, has to go to the normal
    // search (which can delay it) instead of being passed through with a "nothing beats your
    // click" line while it leads straight into a crash.
    constexpr bool orbClickBeatsAll(bool clickSurvivesLookahead, bool foundBetter) {
        return clickSurvivesLookahead || foundBetter;
    }

    // v2.96 sub-frame orb timings (Click Between Frames). Some orb clicks (the first pink orb
    // in Amethyst, UFO orb chains through a one-block gap) only have a window of a few ticks -
    // under one frame. The whole-tick rules below were built for windows many frames wide:
    //   * the "does this hold up" test shifted the ghost half a tick either way, which is a big
    //     share of a sub-frame window, so a working click read "safe only at the exact spot";
    //   * re-timing refused the last tick you still touch the orb on, and with a 1-2 tick
    //     window that left no candidate at all.
    // With CBF the click you made already sits at a real sub-tick position (0.43 of a tick,
    // ~0.11 frames), so that position is used as it is instead of being guessed at.
    constexpr bool subFrameWindow(int firstTouch, int lastTouch, int ticksPerFrame) {
        return firstTouch >= 0 && lastTouch >= firstTouch && (lastTouch - firstTouch) < std::max(1, ticksPerFrame);
    }

    // Half a tick of movement is a huge share of a sub-frame window: use a quarter inside it.
    inline float orbSafetyShift(float halfTickShift, bool subFrame) {
        return subFrame ? std::max(0.15f, halfTickShift * 0.5f) : halfTickShift;
    }

    // A CBF click lands inside the tick, so the last tick you still touch the orb on can be
    // used when the click sits in the middle of its tick. Whole-tick predictions never use it
    // (a fraction of a tick either way would put the press outside the orb).
    constexpr bool lastOrbTickUsable(bool cbfSync, bool subFrame, float clickFraction) {
        return cbfSync && subFrame && clickFraction >= 0.08f && clickFraction <= 0.92f;
    }

    // The click is already at a real sub-tick position, so the half-tick "safe only at the
    // exact spot" re-search must not move it: it would retime a working sub-frame timing.
    constexpr bool keepSubTickClick(bool cbfSync, bool subFrame, bool midTick) {
        return cbfSync && subFrame && midTick;
    }

    // v2.96 UFO: an orb press is judged on its own path. The "you could have flapped again"
    // excuse made every UFO orb click read safe (some later flap could always recover), so the
    // click was never re-timed and the sub-frame timing that gets through the gap was lost.
    constexpr bool pressUsedOrb(int usedOrbTick, int withinTicks) {
        return usedOrbTick >= 0 && usedOrbTick <= withinTicks;
    }

    template <typename Player>
    bool invertedWave(Player const* p1, Player const* p2, bool dual) {
        return (p1 && p1->m_isDart && p1->m_isUpsideDown) ||
            (dual && p2 && p2->m_isDart && p2->m_isUpsideDown);
    }

    inline int waveSpamGap(int configuredGap, bool invertedWave, int invertedGap) {
        return invertedWave ? std::min(configuredGap, invertedGap) : configuredGap;
    }

    // v2.95: inverted wave is no longer judged twice as strictly, and a gravity portal in the
    // middle of a prediction no longer turns the whole run into a "strict recovery" (that made
    // the upside-down <-> normal transitions the player wants to spam through the hardest part
    // to buffer). Inverted wave keeps a small extra margin (see the call site) instead.
    constexpr bool strictWaveRecovery(bool startedInverted, bool gravityChanged) {
        (void)gravityChanged;
        return startedInverted;
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

    // v2.95: ship buffering is deliberately conservative. A delayed ship input changes every
    // tick of the line after it, so delaying a tap "because the crash moves later" was what flew
    // the ship up or down into the next obstacle. A ship delay now has to either clear the whole
    // look-ahead, or be a short tap (at most 2 frames at 240 ticks) whose crash genuinely moves
    // further out. Straight-fly rhythms are never retimed for a marginal gain.
    constexpr int SHIP_MAX_TAP_DELAY = 8;  // ticks (2 frames at 60 fps)

    constexpr int SHIP_CRASH_MARGIN = 12;  // the crash has to move at least this much further

    constexpr bool shipDelayAccepted(bool straightFly, int delay, int pressNowCrash, int delayedCrash, int horizon,
        int maxTapDelay, int margin) {
        if (delayedCrash >= horizon) {
            return true;  // clears the whole look-ahead: a real save
        }
        if (straightFly || pressNowCrash >= horizon || delay > maxTapDelay) {
            return false;
        }
        // Both crashes measured from now: pressing now crashes at pressNowCrash, the delayed
        // tap crashes `delay` ticks later, at delay + delayedCrash.
        return delay + delayedCrash >= pressNowCrash + margin;
    }

}
