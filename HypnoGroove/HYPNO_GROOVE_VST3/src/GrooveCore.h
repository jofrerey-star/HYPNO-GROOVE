#pragma once

// Dependency-free core shared by the VST3 and offline test/preview.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hypno {
constexpr int lanes = 6;
constexpr int steps = 16;
constexpr double pi = 3.14159265358979323846;

enum Lane : int { Kick, Clap, ClosedHat, OpenHat, Shaker, Rim };
inline constexpr std::array<const char*, lanes> laneNames = {
    "KICK", "CLAP", "CLOSED HAT", "OPEN HAT", "SHAKER", "RIM"
};

inline uint16_t mask(std::initializer_list<int> stepNumbers) {
    uint16_t result = 0;
    for (int step : stepNumbers)
        if (step >= 1 && step <= 16) result |= uint16_t(1u << (step - 1));
    return result;
}

inline std::array<uint16_t, lanes> patternA() {
    return { mask({1,5,9,13}), mask({5,13}), mask({3,7,11,15}),
             mask({7,15}), mask({2,4,6,8,10,12,14,16}), mask({12}) };
}
inline std::array<uint16_t, lanes> patternB() {
    return { mask({1,5,9,13,16}), mask({5,13}), mask({3,7,10,11,15}),
             mask({7,15}), mask({2,4,6,8,10,12,14}), mask({6,14}) };
}

inline bool isActive(uint16_t maskValue, int zeroBasedStep) {
    return zeroBasedStep >= 0 && zeroBasedStep < 16
            && (maskValue & (uint16_t(1u << zeroBasedStep))) != 0;
}
inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16; return x;
}
inline float unitRandom(uint32_t seed) { return float(hash32(seed) & 0xffffffu) / 16777216.0f; }
// Each step is a 16th note (0.25 quarter-note beats).
// Swing postpones odd 16ths without crossing the next step.
inline double stepPpq(int64_t absoluteStep, float swingPercent) {
    return double(absoluteStep) * 0.25 + (absoluteStep % 2 != 0 ?
        0.25 * (std::clamp(swingPercent, 0.0f, 100.0f) / 100.0f) * 0.48 : 0.0);
}
inline float stepVelocity(int64_t absoluteStep, int lane, float humanizePercent) {
    const auto seed = uint32_t(absoluteStep) * 1315423911u + uint32_t(lane) * 2654435761u;
    const float deviation = (unitRandom(seed) * 2.0f - 1.0f)
                            * std::clamp(humanizePercent, 0.0f, 100.0f) * 0.0035f;
    return std::clamp(0.85f + deviation, 0.15f, 1.0f);
}
inline float noise(uint32_t& state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return float(state & 0xffffu) / 32768.0f - 1.0f;
}

// Generates royalty-free drum hits, so the plugin makes sound immediately,
// even without external sample files. Users can replace each lane with WAV/AIFF.
inline std::vector<float> makeFactoryHit(int lane, double sampleRate) {
    const double seconds[] = { .44, .31, .11, .48, .12, .18 };
    const auto duration = seconds[std::clamp(lane, 0, lanes - 1)];
    const auto count = std::max(1, int(std::round(sampleRate * duration)));
    std::vector<float> result(size_t(count), 0.0f);
    uint32_t rng = 0x7f4a7c15u + uint32_t(lane) * 10007u;
    double phase = 0.0, lowpass = 0.0;
    for (int i = 0; i < count; ++i) {
        const double t = i / sampleRate;
        const double n = noise(rng);
        lowpass += 0.13 * (n - lowpass);
        const double high = n - lowpass;
        double value = 0.0;
        switch (lane) {
        case Kick: {
            const double freq = 49.0 + 115.0 * std::exp(-t * 33.0);
            phase += 2.0 * pi * freq / sampleRate;
            value = std::sin(phase) * std::exp(-t * 14.0) * (1.0 - std::exp(-t * 1400.0));
            value += 0.13 * n * std::exp(-t * 230.0);
            break;
        }
        case Clap: {
            const double burst = std::exp(-std::pow(t / .003, 2.0))
                + 0.85 * std::exp(-std::pow((t - .015) / .004, 2.0))
                + 0.75 * std::exp(-std::pow((t - .029) / .005, 2.0));
            value = high * (0.65 * burst + 0.24 * std::exp(-t * 16.0));
            break;
        }
        case ClosedHat:
            value = high * std::exp(-t * 49.0); break;
        case OpenHat:
            value = high * std::exp(-t * 10.5); break;
        case Shaker:
            value = high * std::exp(-t * 37.0) * (0.6 + 0.4 * std::abs(std::sin(t * 500.0))); break;
        case Rim:
            value = (0.46 * std::sin(2*pi*880*t) + 0.3 * std::sin(2*pi*1730*t))
                    * std::exp(-t * 37.0) + 0.28 * high * std::exp(-t * 95.0); break;
        default: break;
        }
        result[size_t(i)] = float(value);
    }
    float peak = 0.0f;
    for (float sample : result) peak = std::max(peak, std::abs(sample));
    if (peak > 0.0001f) for (auto& sample : result) sample *= 0.82f / peak;
    return result;
}
} // namespace hypno
