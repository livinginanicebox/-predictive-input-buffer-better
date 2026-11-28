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
#include <string>
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
