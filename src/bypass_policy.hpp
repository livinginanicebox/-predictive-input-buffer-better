#pragma once

// v2.96: hold Tab to step out of the buffer's way. While Tab is held the buffer
// predicts nothing, holds nothing back and drops what it was waiting to deliver,
// so the game gets exactly the inputs you make - and the moment you let go the
// buffer is back with the stage you had before. Nothing on screen says so.
namespace bypass_policy {
    constexpr bool bufferActive(bool stageEnabled, bool bypassHeld) {
        return stageEnabled && !bypassHeld;
    }

    struct State {
        bool held = false;

        // One keyboard event. Returns true when the bypass state changed, so the
        // caller only reacts once per press / release. Auto-repeat while Tab is
        // held is not a new press and never ends the bypass.
        bool feed(bool isBypassKey, bool isKeyDown, bool isRepeat) {
            if (!isBypassKey || isRepeat) {
                return false;
            }
            bool next = isKeyDown;
            if (held == next) {
                return false;
            }
            held = next;
            return true;
        }

        // The key-up never arrives if the game loses focus while the key is held
        // (alt-tab), which would leave the buffer bypassed for good. The physics
        // step polls the key itself, and a poll can end the bypass as well.
        bool poll(bool osSaysDown) {
            if (held == osSaysDown) {
                return false;
            }
            held = osSaysDown;
            return true;
        }

        // A level start / restart / menu is not a deliberate bypass.
        void reset() {
            held = false;
        }
    };
}
