#include "../src/motion_policy.hpp"
#include <cassert>

int main() {
    using namespace motion_policy;

    // Movement is measured in world units between two physics steps, not guessed
    // from the nominal speed portal value.
    assert(unitsBetween(100.0, 10.0, 101.3, 11.4) > 1.6);
    assert(unitsBetween(5.0, 5.0, 5.0, 5.0) == 0.0);
    assert(!movingInUnits(0.01));  // float jitter is not a moving saw
    assert(movingInUnits(1.3));    // 0.9 speed at 240 Hz

    // The median ignores a single snapped/duplicated step.
    std::array<double, 4> samples { 1.3, 9.0, 1.4, 1.2 };
    assert(steadyUnits(samples, 4) > 1.25 && steadyUnits(samples, 4) < 1.4);
    assert(steadyUnits(samples, 0) == 0.0);

    // Robust slack is half of the distance actually moved in one step.
    assert(halfStepShift(1.3f) > 0.6f && halfStepShift(1.3f) < 0.7f);
    assert(halfStepShift(0.f) == 0.f);
    assert(halfStepShift(-2.f) == 0.f);
    assert(halfStepShift(1000.f) == MAX_SHIFT_UNITS);
    assert(fallbackShift(0.9f) > 0.6f && fallbackShift(0.9f) < 0.7f);
    assert(robustShift(0.f, 1.6f) > robustShift(0.f, 0.7f));
    assert(robustShift(2.f, 0.7f) == halfStepShift(2.f));

    // Extrapolating a single measured velocity too far is a guess, not prediction.
    assert(reliableSteps(1.3, 1000) < 1000);
    assert(reliableSteps(0.0, 1000) == 1000);
    assert(projectedUnits(2.0, 10) == 20.0);
    assert(projectedUnits(0.0, 10) == 0.0);
    // A saw turning a full circle every 8 steps returns to its starting point.
    assert(projectedUnits(1.0, 8, 6.283185307179586 / 8.0) < 0.001);
    assert(projectedUnits(1.0, 4, 6.283185307179586 / 8.0) > 2.5);
}
