// Unit tests for the macro timing parser (v3.4). Build like the other test files:
//   g++ -std=c++23 -O1 -Wall -Wextra -o /tmp/macro_test tests/macro_policy_test.cpp && /tmp/macro_test
#include "../src/macro_policy.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

int main() {
    using namespace macro_policy;

    // --- numbers(...) -------------------------------------------------------------
    {
        auto v = numbers("12, 34;-56\t78");
        assert(v.size() == 4);
        assert(v[0] == 12 && v[1] == 34 && v[2] == -56 && v[3] == 78);
        assert(numbers("no digits here").empty());
        assert(numbers("").empty());
    }

    // --- parseLine: tick,down -----------------------------------------------------
    {
        Click c;
        assert(parseLine("1234,1", 240, c));
        assert(c.tick == 1234 && c.down && c.player1);

        assert(parseLine("1240,0", 240, c));
        assert(c.tick == 1240 && !c.down && c.player1);

        // 60 fps macro: 30 frames = 120 ticks at 240/s
        assert(parseLine("30,1", 60, c));
        assert(c.tick == 120 && c.down);

        // tick,down,player
        assert(parseLine("900,1,2", 240, c));
        assert(c.tick == 900 && c.down && !c.player1);

        // tick,x,y,down layout
        assert(parseLine("480,120.5,-99.25,0", 240, c));
        assert(c.tick == 480 && !c.down);

        // a single number is not a click
        assert(!parseLine("480", 240, c));
        assert(!parseLine("", 240, c));
        // negative or absurd times are rejected
        assert(!parseLine("-5,1", 240, c));
        assert(!parseLine("999999999,1", 240, c));
    }

    // --- parse(...) ---------------------------------------------------------------
    {
        std::string text =
            "# my macro\n"
            "// another comment\n"
            "\n"
            "240,1\r\n"
            "300,0\n"
            "garbage line\n"
            "480,1\n";
        int read = 0;
        auto clicks = parse(text, 240, &read);
        assert(clicks.size() == 3);
        assert(read == 3);
        assert(clicks[0].tick == 240 && clicks[0].down);
        assert(clicks[1].tick == 300 && !clicks[1].down);
        assert(clicks[2].tick == 480 && clicks[2].down);
    }

    // unsorted input comes back sorted
    {
        auto clicks = parse("900,1\n100,1\n500,0\n", 240);
        assert(clicks.size() == 3);
        assert(clicks[0].tick == 100);
        assert(clicks[1].tick == 500);
        assert(clicks[2].tick == 900);
    }

    // --- nearest(...) -------------------------------------------------------------
    {
        auto clicks = parse("1000,1\n1050,0\n1100,1,2\n", 240);

        // a press 3 ticks early finds the macro press
        assert(nearest(clicks, 997, true, true, 24) == 1000);
        // a press 3 ticks late finds it too
        assert(nearest(clicks, 1003, true, true, 24) == 1000);
        // out of the window -> nothing
        assert(nearest(clicks, 900, true, true, 24) == INT_MIN);
        // direction matters: no release at 1000
        assert(nearest(clicks, 1000, false, true, 24) == INT_MIN);
        // the release at 1050
        assert(nearest(clicks, 1046, false, true, 24) == 1050);
        // player 2 only
        assert(nearest(clicks, 1101, true, false, 24) == 1100);
        assert(nearest(clicks, 1101, true, true, 24) == INT_MIN);
        // a window of zero never matches
        assert(nearest(clicks, 1000, true, true, 0) == INT_MIN);
    }

    // nearest picks the closest of two candidates
    {
        auto clicks = parse("1000,1\n1010,1\n", 240);
        assert(nearest(clicks, 1004, true, true, 24) == 1000);
        assert(nearest(clicks, 1007, true, true, 24) == 1010);
    }

    // --- remaining(...) -----------------------------------------------------------
    {
        auto clicks = parse("100,1\n200,1\n300,1\n", 240);
        assert(remaining(clicks, 0) == 3);
        assert(remaining(clicks, 200) == 2);
        assert(remaining(clicks, 301) == 0);
    }


    // --- .gdr2 replay reader ------------------------------------------------------
    {
        // builds a GDR2 v2 file: header, metadata, then (delta,down) inputs at `fps`
        auto build = [](double fps, std::vector<std::pair<int, bool>> inputs, std::string levelName) {
            std::vector<unsigned char> out;
            auto varint = [&](unsigned long long v) {
                do {
                    unsigned char b = static_cast<unsigned char>(v & 0x7f);
                    v >>= 7;
                    if (v) {
                        b |= 0x80;
                    }
                    out.push_back(b);
                } while (v);
            };
            auto string = [&](std::string const& s) {
                varint(s.size());
                out.insert(out.end(), s.begin(), s.end());
            };
            auto be32 = [&](std::uint32_t bits) {
                for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<unsigned char>((bits >> s) & 0xff));
            };
            auto be64 = [&](std::uint64_t bits) {
                for (int s = 56; s >= 0; s -= 8) out.push_back(static_cast<unsigned char>((bits >> s) & 0xff));
            };
            out.push_back('G');
            out.push_back('D');
            out.push_back('R');
            varint(2);
            string("");      // input extensions
            string("tester");  // author
            string("");        // description
            be32(0x3f800000u);  // duration 1.0 s
            varint(22000);      // game version
            std::uint64_t fpsBits = 0;
            double f = fps;
            std::memcpy(&fpsBits, &f, sizeof(fpsBits));
            be64(fpsBits);
            varint(0);   // seed
            varint(0);   // coins
            varint(0);   // ldm
            varint(0);   // platformer
            string("Bot");
            varint(1);   // bot version
            varint(4242);  // level id
            string(levelName);
            varint(0);   // replay extensions
            varint(0);   // deaths
            varint(inputs.size());
            varint(inputs.size());  // all player 1
            for (auto const& [delta, down] : inputs) {
                varint((static_cast<unsigned long long>(delta) << 1) | (down ? 1u : 0u));
            }
            return out;
        };

        auto data = build(240.0, { { 100, true }, { 40, false }, { 0, true } }, "Test Level");
        assert(looksLikeGdr2(data));
        auto replay = parseGdr2(data);
        assert(replay.ok);
        assert(replay.version == 2);
        assert(replay.framerate == 240);
        assert(replay.levelName == "Test Level");
        assert(replay.author == "tester");
        assert(replay.botName == "Bot");
        assert(replay.clicks.size() == 3);
        assert(replay.clicks[0].tick == 100 && replay.clicks[0].down && replay.clicks[0].player1);
        assert(replay.clicks[1].tick == 140 && !replay.clicks[1].down);
        assert(replay.clicks[2].tick == 140 && replay.clicks[2].down);

        // a 60 fps replay is converted to the game's 240 ticks/s
        auto slow = parseGdr2(build(60.0, { { 10, true }, { 5, false } }, "Slow"));
        assert(slow.ok && slow.framerate == 60);
        assert(slow.clicks.size() == 2);
        assert(slow.clicks[0].tick == 40 && slow.clicks[0].down);
        assert(slow.clicks[1].tick == 60 && !slow.clicks[1].down);

        // every input is read even when the file's own count is wrong: rewrite the count as 2
        auto miscounted = data;
        // the input count sits right after the death count; find it by searching for the
        // (inputCount, p1Count) pair written as single bytes 3,3 and replace with 2,2
        for (size_t i = 0; i + 1 < miscounted.size(); ++i) {
            if (miscounted[i] == 3 && miscounted[i + 1] == 3) {
                miscounted[i] = 2;
                miscounted[i + 1] = 2;
                break;
            }
        }
        auto recovered = parseGdr2(miscounted);
        assert(recovered.ok);
        assert(recovered.declaredInputs == 2);
        assert(recovered.readInputs == 3);      // read to the end of the input block
        assert(recovered.clicks.size() == 3);
        for (auto const& click : recovered.clicks) {
            assert(click.player1);              // a wrong header can never make the file player 2
        }
        assert(recovered.clicks[2].tick == replay.clicks[2].tick);  // same timing as the good read

        // truncated file -> fewer clicks or a clean error, never a crash
        auto cut = data;
        cut.resize(cut.size() - 2);
        auto broken = parseGdr2(cut);
        assert(!broken.ok || broken.clicks.size() < 3);

        // a text file is not a replay
        std::vector<unsigned char> text { 'h', 'i', '\n' };
        assert(!looksLikeGdr2(text));
        assert(!parseGdr2(text).ok);
    }

    // --- .gdr2 version 1 (bit-packed inputs, NUL-terminated text) ------------------
    {
        std::vector<unsigned char> out;
        auto varint = [&](unsigned long long v) {
            do {
                unsigned char b = static_cast<unsigned char>(v & 0x7f);
                v >>= 7;
                if (v) {
                    b |= 0x80;
                }
                out.push_back(b);
            } while (v);
        };
        auto cstring = [&](std::string const& s) {
            out.insert(out.end(), s.begin(), s.end());
            out.push_back(0);
        };
        out.push_back('G');
        out.push_back('D');
        out.push_back('R');
        varint(1);            // version
        cstring("");          // input tag
        cstring("old");       // author
        cstring("");          // description
        varint(1000);         // duration in frames
        varint(22000);        // game version
        varint(240);          // framerate
        varint(0);            // seed
        varint(0);            // coins
        varint(0);            // ldm
        varint(0);            // platformer
        cstring("OldBot");
        varint(1);            // bot version
        varint(7);            // level id
        cstring("V1 Level");
        varint(0);            // replay extensions
        varint(0);            // deaths
        varint(2);            // inputs
        varint(2);            // all player 1
        out.push_back(static_cast<unsigned char>((5 << 2) | 0x02));   // delta 5, down
        out.push_back(static_cast<unsigned char>((10 << 2)));         // delta 10, up

        auto replay = parseGdr2(out);
        assert(replay.ok);
        assert(replay.version == 1);
        assert(replay.levelName == "V1 Level");
        assert(replay.botName == "OldBot");
        assert(replay.clicks.size() == 2);
        assert(replay.clicks[0].tick == 5 && replay.clicks[0].down);
        assert(replay.clicks[1].tick == 15 && !replay.clicks[1].down);
    }

    std::printf("macro_policy tests passed\n");
    return 0;
}
