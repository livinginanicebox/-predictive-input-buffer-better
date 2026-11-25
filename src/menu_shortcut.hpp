#pragma once

namespace menu_shortcut {
    // v2.95: the menu opens with four quick F3 taps instead of one press, so a
    // single F3 (or a stray press while playing) no longer pauses the level and
    // opens a popup. A tap that comes later than GAP_MS starts the count over.
    constexpr int PRESSES = 4;
    constexpr double GAP_MS = 400.0;

    struct Counter {
        int presses = 0;
        double lastMs = 0.0;
        bool started = false;

        // One keyboard event. `opened` is set when this event completes the
        // sequence; the return value is the press count in the sequence now
        // (0 when the event didn't count, or right after the menu opened).
        int feed(bool isF3, bool isKeyDown, bool isRepeat, bool inLevel, double nowMs, bool& opened) {
            opened = false;
            if (!isF3 || !isKeyDown || isRepeat || !inLevel) {
                return presses;
            }
            // `nowMs >= lastMs` guards against a clock that isn't monotonic.
            bool inTime = started && nowMs >= lastMs && nowMs - lastMs <= GAP_MS;
            presses = inTime ? presses + 1 : 1;
            lastMs = nowMs;
            started = true;
            if (presses >= PRESSES) {
                presses = 0;
                started = false;
                opened = true;
            }
            return presses;
        }
    };
}
