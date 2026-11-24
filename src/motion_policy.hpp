#pragma once

#include <algorithm>
#include <array>
#include <cmath>

// v2.95: prediction used to assume how far a step moves (311.58 units/s / 240 Hz,
// scaled by the speed portal value). That is wrong at unlocked FPS, with speed
// changes, slopes and CBF. These helpers use the distance actually measured in
// units per physics step instead.
namespace motion_policy {
    constexpr float MIN_UNITS_PER_STEP = 0.02f;  // below this an object is standing still (float jitter)
    constexpr float MAX_SHIFT_UNITS = 8.f;       // never test a save half a screen away
    constexpr double MAX_PROJECT_UNITS = 600.0;  // cap for extrapolating a moving object

    inline double unitsBetween(double x0, double y0, double x1, double y1) {
        double const dx = x1 - x0;
        double const dy = y1 - y0;
        return std::hypot(dx, dy);
    }

    // A single step is noisy (objects snap to their path, CBF splits a step, triggers
    // fire late). Take the median of the last few real displacements instead.
    template <size_t N>
    double steadyUnits(std::array<double, N> const& samples, int count) {
        std::array<double, N> sorted {};
        int n = std::clamp(count, 0, static_cast<int>(N));
        for (int i = 0; i < n; ++i) {
            sorted[i] = samples[i];
        }
        std::sort(sorted.begin(), sorted.begin() + n);
        if (n == 0) {
            return 0.0;
        }
        return (n & 1) ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) * 0.5;
    }

    inline bool movingInUnits(double unitsPerStep) {
        return std::isfinite(unitsPerStep) && unitsPerStep >= MIN_UNITS_PER_STEP;
    }

    // Half a step of slack, in world units, for the robust-save test.
    inline float halfStepShift(float unitsPerStep) {
        if (!std::isfinite(unitsPerStep) || unitsPerStep <= 0.f) {
            return 0.f;
        }
        return std::clamp(unitsPerStep * 0.5f, 0.3f, MAX_SHIFT_UNITS);
    }

    // Only used before the first real step has been measured.
    inline float fallbackShift(float playerSpeed) {
        return std::max(0.3f, 0.5f * (311.58f / 240.f) * (std::abs(playerSpeed) / 0.9f));
    }

    inline float robustShift(float unitsPerStep, float playerSpeed) {
        float measured = halfStepShift(unitsPerStep);
        return measured > 0.f ? measured : fallbackShift(playerSpeed);
    }

    // How many steps of a moving object's current velocity are still inside the
    // part of the level a prediction can reach. Long extrapolations of a single
    // measured velocity are guesses, not prediction.
    inline int reliableSteps(double unitsPerStep, int steps) {
        if (!movingInUnits(unitsPerStep)) {
            return steps;
        }
        int limit = static_cast<int>(std::floor(MAX_PROJECT_UNITS / unitsPerStep));
        return std::max(0, std::min(steps, limit));
    }

    // Distance travelled in `steps` while the direction turns `turn` radians per step.
    inline double projectedUnits(double unitsPerStep, int steps, double turn = 0.0) {
        if (!movingInUnits(unitsPerStep) || steps <= 0) {
            return 0.0;
        }
        if (std::abs(turn) < 1e-6) {
            return unitsPerStep * steps;
        }
        double half = turn * 0.5;
        if (std::abs(std::sin(half)) < 1e-6) {
            return 0.0;
        }
        return std::abs(unitsPerStep * std::sin(steps * half) / std::sin(half));
    }
}
