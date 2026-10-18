#include "../src/GrooveCore.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace hypno;
    auto a = patternA(); auto b = patternB();
    assert(isActive(a[Kick], 0));
    assert(!isActive(a[Kick], 1));
    assert(isActive(a[Clap], 4));
    assert(isActive(b[Kick], 15));
    assert(!isActive(a[Kick], 15));
    assert(std::abs(stepPpq(1, 0) - 0.25) < 1e-6);
    assert(stepPpq(1, 50) > stepPpq(1, 0));
    assert(stepPpq(1, 100) < stepPpq(2, 0));
    assert(stepPpq(17, 50) > stepPpq(17, 0));
    assert(stepVelocity(8, Kick, 0) == stepVelocity(8, Kick, 0));
    assert(stepVelocity(8, Kick, 100) >= .15f && stepVelocity(8, Kick, 100) <= 1.f);
    for (int lane = 0; lane < lanes; ++lane) {
        const auto sound = makeFactoryHit(lane, 44100);
        assert(sound.size() > 1000);
        float peak = 0.f; for (float s : sound) peak = std::max(peak, std::abs(s));
        assert(peak > .25f && peak <= 1.0f);
    }
    std::cout << "PASS: patterns, swing, velocity and six factory sounds\n";
}
