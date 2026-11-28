// Macro timing (v3.4): the timings from a macro of the level you are playing.
//
// A macro is a list of clicks with the time they happen at. The mod reads it from
// <mod save dir>/macro.txt and, while "Macro timing" is on, a real click that is a little
// early is delivered exactly on the macro's tick instead of on the tick the prediction
// guesses. That is the one thing a prediction cannot know and a macro can.
//
// Accepted line layouts (numbers separated by commas, semicolons, spaces or tabs):
//   tick,down                for example  1234,1   (1234,0 = release)
//   tick,down,player         for example  1234,1,2
//   tick,anything,down       any extra column is ignored, a 0/1 column is read as the button
//   nothing, x, y, down      columns after the third are only used when the third is not 0/1
// Blank lines, lines starting with '#' and lines starting with "//" are ignored.
// Times are in ticks at 'macro-fps' (default 240 = 1/240 s); 60 fps macros are converted.
#pragma once

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace macro_policy {
    struct Click {
        int tick = 0;
        bool down = true;
        bool player1 = true;
    };

    // Every number on a line, in order. A line with no numbers yields an empty vector.
    inline std::vector<long long> numbers(std::string const& line) {
        std::vector<long long> out;
        size_t i = 0;
        while (i < line.size()) {
            bool negative = false;
            if (line[i] == '-' && i + 1 < line.size() && std::isdigit(static_cast<unsigned char>(line[i + 1]))) {
                negative = true;
                ++i;
            }
            if (!std::isdigit(static_cast<unsigned char>(line[i]))) {
                ++i;
                continue;
            }
            long long value = 0;
            while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
                value = value * 10 + (line[i] - '0');
                ++i;
            }
            out.push_back(negative ? -value : value);
        }
        return out;
    }

    inline bool isButton(long long value) {
        return value == 0 || value == 1;
    }

    // One line -> one click. `fps` converts the line's time to the game's 240 ticks/s.
    inline bool parseLine(std::string const& line, int fps, Click& click) {
        auto values = numbers(line);
        if (values.size() < 2) {
            return false;
        }
        if (fps <= 0) {
            fps = 240;
        }
        double ticks = static_cast<double>(values[0]) * 240.0 / static_cast<double>(fps);
        if (ticks < 0.0 || ticks > 240.0 * 60.0 * 60.0) {
            return false;
        }
        click.tick = static_cast<int>(std::llround(ticks));
        // A player column is written 1 / 2 (2 = player 2), a button column 0 / 1.
        auto player = [](std::vector<long long> const& v, size_t index) {
            return index >= v.size() || v[index] != 2;
        };
        if (isButton(values[1])) {
            click.down = values[1] == 1;
            click.player1 = player(values, 2);
            return true;
        }
        // tick,x,y,down layouts: the first 0/1 column after the position is the button
        for (size_t i = 2; i < values.size(); ++i) {
            if (isButton(values[i])) {
                click.down = values[i] == 1;
                click.player1 = player(values, i + 1);
                return true;
            }
        }
        return false;
    }

    // Whole file -> sorted clicks. Duplicate ticks keep the last one seen.
    inline std::vector<Click> parse(std::string const& text, int fps, int* linesRead = nullptr) {
        std::vector<Click> clicks;
        if (linesRead) {
            *linesRead = 0;
        }
        size_t start = 0;
        while (start <= text.size()) {
            size_t end = text.find('\n', start);
            std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            size_t first = line.find_first_not_of(" \t");
            bool comment = first != std::string::npos && (line[first] == '#' || (line[first] == '/' && first + 1 < line.size() && line[first + 1] == '/'));
            Click click;
            if (!comment && parseLine(line, fps, click)) {
                clicks.push_back(click);
                if (linesRead) {
                    ++*linesRead;
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        std::stable_sort(clicks.begin(), clicks.end(), [](Click const& a, Click const& b) { return a.tick < b.tick; });
        return clicks;
    }

    // The macro click of the same direction nearest to `tick`, at most `window` ticks away.
    // Returns INT_MIN when nothing is close enough.
    inline int nearest(std::vector<Click> const& clicks, int tick, bool down, bool player1, int window) {
        if (window <= 0) {
            return INT_MIN;
        }
        int best = INT_MIN;
        int bestDistance = window + 1;
        for (auto const& click : clicks) {
            if (click.down != down || click.player1 != player1) {
                continue;
            }
            int distance = std::abs(click.tick - tick);
            if (distance <= window && distance < bestDistance) {
                bestDistance = distance;
                best = click.tick;
            }
        }
        return best;
    }


    // ---------------------------------------------------------------------------------------
    // .gdr2 replays (GDR2, what GDShare / Echo / EclipseBot write).
    //
    // Layout (all integers unsigned LEB128, floats big-endian):
    //   "GDR" varint(version)  string(inputTag)
    //   string(author) string(description) f32(durationSeconds) varint(gameVersion)
    //   f64(framerate) varint(seed) varint(coins) bool(ldm) bool(platformer)
    //   string(botName) varint(botVersion) varint(levelId) string(levelName)
    //   varint(extensionSize) skip(extensionSize)
    //   varint(deathCount) varint(delta)*
    //   varint(inputCount) varint(player1Count)
    //   player1 inputs, then player2 inputs, each: varint(delta<<1 | down)
    //   (platformer: varint(delta<<3 | button<<1 | down), plus a varint extension size when
    //    inputTag is not empty; version 1 files are bit-packed instead - both are read)
    // Frames are counted at the replay's framerate, which is converted to the game's 240 ticks/s.
    // ---------------------------------------------------------------------------------------
    struct Replay {
        std::vector<Click> clicks;
        int framerate = 240;
        int version = 0;
        std::string levelName;
        std::string author;
        std::string botName;
        std::string description;
        std::string error;
        bool ok = false;
        bool consumedAll = false;        // the parse used the whole file
        std::uint64_t declaredInputs = 0;  // inputs the file says it has
        std::uint64_t readInputs = 0;      // inputs actually read
        std::uint64_t skippedInputs = 0;   // left/right platformer inputs (not jump)
    };

    inline bool looksLikeGdr2(std::vector<unsigned char> const& data) {
        return data.size() >= 4 && data[0] == 'G' && data[1] == 'D' && data[2] == 'R';
    }

    class Gdr2Reader {
    public:
        explicit Gdr2Reader(std::vector<unsigned char> const& data) : data_(data) {}

        bool failed = false;
        std::string error;
        size_t pos = 0;

        bool done() const {
            return failed || pos >= data_.size();
        }

        size_t remaining() const {
            return pos < data_.size() ? data_.size() - pos : 0;
        }

        void fail(std::string why) {
            if (!failed) {
                failed = true;
                error = std::move(why);
            }
        }

        unsigned char byte() {
            if (done() || pos >= data_.size()) {
                fail("unexpected end of file");
                return 0;
            }
            return data_[pos++];
        }

        std::uint64_t varint() {
            std::uint64_t value = 0;
            for (unsigned shift = 0; shift < 64; shift += 7) {
                unsigned char b = byte();
                if (failed) {
                    return 0;
                }
                value |= static_cast<std::uint64_t>(b & 0x7f) << shift;
                if ((b & 0x80) == 0) {
                    return value;
                }
            }
            fail("varint is too long");
            return 0;
        }

        std::uint64_t count(std::uint64_t limit, char const* what) {
            std::uint64_t value = varint();
            if (!failed && value > limit) {
                fail(std::string(what) + " is unreasonable");
            }
            return failed ? 0 : value;
        }

        bool boolean() {
            return varint() != 0;
        }

        std::string string() {
            std::uint64_t length = count(static_cast<std::uint64_t>(remaining()), "string length");
            if (failed) {
                return {};
            }
            std::string text(reinterpret_cast<char const*>(data_.data() + pos), static_cast<size_t>(length));
            pos += static_cast<size_t>(length);
            return text;
        }

        std::string cstring() {
            size_t start = pos;
            for (;;) {
                if (byte() == 0 || failed) {
                    break;
                }
                if (pos - start > 4096) {
                    fail("text field is too long");
                    break;
                }
            }
            if (failed) {
                return {};
            }
            return std::string(reinterpret_cast<char const*>(data_.data() + start), pos - start - 1);
        }

        float f32() {
            std::uint32_t bits = 0;
            for (int i = 0; i < 4; ++i) {
                bits = (bits << 8) | byte();
            }
            float value = 0.f;
            if (!failed) {
                std::memcpy(&value, &bits, sizeof(value));
            }
            return value;
        }

        double f64() {
            std::uint64_t bits = 0;
            for (int i = 0; i < 8; ++i) {
                bits = (bits << 8) | byte();
            }
            double value = 0.0;
            if (!failed) {
                std::memcpy(&value, &bits, sizeof(value));
            }
            return value;
        }

        void skip(std::uint64_t amount) {
            if (amount > remaining()) {
                fail("unexpected end of file");
                return;
            }
            pos += static_cast<size_t>(amount);
        }

    private:
        std::vector<unsigned char> const& data_;
    };

    // Reads the input block into 240 ticks/s clicks. The reference reader (GDReplayFormat)
    // reads inputs until the end of the file and uses the declared player-1 count only to know
    // where the player 2 list starts - a wrong count in the file must not cost you the clicks,
    // which is exactly what happened when this trusted the declared count instead.
    inline void gdr2Inputs(Gdr2Reader& r, std::uint64_t player1Remaining, bool platformer, bool extensions, bool v1,
        int framerate, std::vector<Click>& out, std::uint64_t* totalIn = nullptr, std::uint64_t* skipped = nullptr) {
        std::uint64_t frame = 0;
        std::uint64_t read = 0;
        while (!r.failed && r.remaining() > 0 && read < 4000000) {
            size_t before = r.pos;
            std::uint64_t delta = 0;
            bool down = false;
            bool jump = true;
            if (v1) {
                unsigned char first = r.byte();
                down = (first & 0x02) != 0;
                bool more = (first & 0x01) != 0;
                if (platformer) {
                    unsigned button = (first >> 2) & 0x3;
                    jump = button == 1;
                    delta = first >> 4;
                }
                else {
                    delta = first >> 2;
                }
                if (more) {
                    delta |= r.varint() << (platformer ? 4 : 6);
                }
            }
            else {
                std::uint64_t packed = r.varint();
                down = (packed & 1) != 0;
                if (platformer) {
                    jump = ((packed >> 1) & 0x3) == 1;
                    delta = packed >> 3;
                }
                else {
                    delta = packed >> 1;
                }
                if (extensions) {
                    std::uint64_t size = r.varint();
                    r.skip(size);
                }
            }
            if (r.failed || r.pos <= before) {
                break;
            }
            ++read;
            bool player2 = player1Remaining == 0;
            bool lastPlayer1 = false;
            if (player1Remaining > 0) {
                --player1Remaining;
                lastPlayer1 = player1Remaining == 0;
            }
            frame += delta;
            if (jump) {
                double ticks = framerate > 0 ? static_cast<double>(frame) * 240.0 / static_cast<double>(framerate) : double(frame);
                out.push_back(Click { static_cast<int>(std::llround(ticks)), down, !player2 });
            }
            else if (skipped) {
                ++*skipped;
            }
            if (lastPlayer1) {
                frame = 0;  // the player 2 list starts at frame 0
            }
        }
        if (totalIn) {
            *totalIn = read;
        }
    }

    // One reading of the file. `durationIsDouble` and `forceV2` cover the variants seen in the
    // wild (a writer using a 64-bit duration, or a file claiming version 1 with the v2 layout).
    inline Replay parseGdr2Layout(std::vector<unsigned char> const& data, bool durationIsDouble, bool forceV2) {
        Replay replay;
        if (!looksLikeGdr2(data)) {
            replay.error = "not a .gdr2 replay (no GDR signature)";
            return replay;
        }
        Gdr2Reader r(data);
        r.pos = 3;
        std::uint64_t version = r.varint();
        replay.version = static_cast<int>(version);
        if (version == 0 || version > 2) {
            replay.error = "unsupported .gdr2 version " + std::to_string(version);
            return replay;
        }
        bool v1 = version == 1 && !forceV2;
        bool platformer = false;
        bool inputExtensions = false;
        std::uint64_t declared = 0;
        std::uint64_t player1 = 0;
        if (v1) {
            std::string tag = r.cstring();
            if (!tag.empty()) {
                replay.error = "this .gdr2 uses input extensions that are not supported";
                return replay;
            }
            replay.author = r.cstring();
            replay.description = r.cstring();
            r.varint();  // duration in frames
            r.varint();  // game version
            replay.framerate = static_cast<int>(r.count(1000000, "framerate"));
            r.varint();  // seed
            r.varint();  // coins
            r.boolean();  // ldm
            platformer = r.boolean();
            replay.botName = r.cstring();
            r.varint();  // bot version
            r.varint();  // level id
            replay.levelName = r.cstring();
            r.skip(r.varint());  // replay extensions
        }
        else {
            std::string tag = r.string();
            inputExtensions = !tag.empty();
            replay.author = r.string();
            replay.description = r.string();
            if (durationIsDouble) {
                r.f64();
            }
            else {
                r.f32();
            }
            r.varint();  // game version
            double framerate = r.f64();
            r.varint();  // seed
            r.varint();  // coins
            r.boolean();  // ldm
            platformer = r.boolean();
            replay.botName = r.string();
            r.varint();  // bot version
            r.varint();  // level id
            replay.levelName = r.string();
            r.skip(r.varint());  // replay extensions
            if (r.failed) {
                replay.error = r.error;
                return replay;
            }
            if (!(framerate > 0.0 && framerate < 1000000.0)) {
                replay.error = "replay has no valid framerate";
                return replay;
            }
            replay.framerate = static_cast<int>(std::llround(framerate));
            if (replay.framerate <= 0) {
                replay.error = "replay framerate rounds to zero";
                return replay;
            }
        }
        if (r.failed) {
            replay.error = r.error;
            return replay;
        }
        // deaths
        std::uint64_t deaths = r.count(1000000, "death count");
        for (std::uint64_t i = 0; i < deaths && !r.failed; ++i) {
            r.varint();
        }
        declared = r.count(4000000, "input count");
        std::uint64_t player1Count = r.count(4000000, "player 1 input count");
        // The input list itself is the truth (it is read to the end of the file); the header counts
        // only decide where the player-2 list starts, and are ignored when they contradict each
        // other so a bad header can never turn the whole replay into player-2 clicks.
        if (declared > 0 && player1Count > 0 && player1Count <= declared) {
            player1 = player1Count;
        }
        else if (declared > 0) {
            player1 = declared;
        }
        else {
            player1 = 4000000;
        }
        if (r.failed) {
            replay.error = r.error;
            return replay;
        }
        replay.declaredInputs = declared;
        std::size_t inputStart = r.pos;
        gdr2Inputs(r, player1, platformer, inputExtensions, v1, replay.framerate, replay.clicks, &replay.readInputs, &replay.skippedInputs);
        if (r.failed) {
            replay.error = r.error;
            return replay;
        }
        if ((replay.readInputs != declared || r.remaining() != 0) && inputStart < data.size()) {
            // The header disagrees with the file, so its player-1/player-2 split cannot be trusted:
            // read the list once more as a single player's, which keeps every click yours.
            std::vector<Click> plain;
            std::uint64_t plainRead = 0;
            std::uint64_t plainSkipped = 0;
            r.pos = inputStart;
            r.failed = false;
            gdr2Inputs(r, 4000000, platformer, inputExtensions, v1, replay.framerate, plain, &plainRead, &plainSkipped);
            if (!r.failed && plain.size() >= replay.clicks.size()) {
                replay.clicks = std::move(plain);
                replay.readInputs = plainRead;
                replay.skippedInputs = plainSkipped;
            }
        }
        // Stable, because two inputs can share a frame and their order decides press vs release.
        std::stable_sort(replay.clicks.begin(), replay.clicks.end(), [](Click const& a, Click const& b) { return a.tick < b.tick; });
        replay.consumedAll = r.remaining() == 0;
        replay.ok = true;
        return replay;
    }

    inline Replay parseGdr2(std::vector<unsigned char> const& data) {
        auto score = [](Replay const& replay) {
            if (!replay.ok) {
                return -1LL;
            }
            long long score = 1000000 + static_cast<long long>(std::min<size_t>(replay.clicks.size(), 200000));
            if (!replay.consumedAll) {
                score -= 500000;  // a reading that stops mid-file is suspicious
            }
            if (replay.readInputs != replay.declaredInputs) {
                score -= 1000;
            }
            return score;
        };
        Replay best = parseGdr2Layout(data, false, false);
        if (score(best) < 1000000 + 100000) {
            for (auto [isDouble, forceV2] : { std::pair<bool, bool> { true, false }, std::pair<bool, bool> { false, true },
                     std::pair<bool, bool> { true, true } }) {
                Replay alt = parseGdr2Layout(data, isDouble, forceV2);
                if (score(alt) > score(best)) {
                    best = std::move(alt);
                }
            }
        }
        if (!best.ok && best.error.empty()) {
            best.error = "could not read the replay";
        }
        return best;
    }

    // How many clicks the macro has after `tick` (used for the load/status log line).
    inline size_t remaining(std::vector<Click> const& clicks, int tick) {
        size_t left = 0;
        for (auto const& click : clicks) {
            if (click.tick >= tick) {
                ++left;
            }
        }
        return left;
    }
}
