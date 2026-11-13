#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace preglow
{
struct ReverbSettings
{
    float decaySeconds = 3.0f;
    float damping = 0.45f;
    float mix = 0.35f;
};

class Reverb
{
public:
    void prepare(double sampleRate)
    {
        constexpr std::array delaySeconds { 0.0297, 0.0371, 0.0411, 0.0437 };
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            line.samples.assign(static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * delaySeconds[i]))), 0.0f);
            line.seconds = static_cast<float>(static_cast<double>(line.samples.size()) / sampleRate);
        }

        for (std::size_t i = 0; i < reflections.size(); ++i)
            reflectionDelays[i] = static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * reflections[i].seconds)));
        earlyHistory.resize(reflectionDelays.back() + 1);

        lastDecay = -1.0f;

        smoothing = static_cast<float>(-std::expm1(-1.0 / (0.025 * sampleRate)));
        reset();
    }

    void reset() noexcept
    {
        for (auto& line : lines)
        {
            std::fill(line.samples.begin(), line.samples.end(), 0.0f);
            line.position = 0;
            line.low = 0.0f;
            line.gain = line.targetGain;
        }
        std::fill(earlyHistory.begin(), earlyHistory.end(), std::array<float, 2> {});
        earlyPosition = 0;
        damping = targetDamping;
        mix = targetMix;
    }

    void setTarget(ReverbSettings settings) noexcept
    {
        const auto decay = bounded(settings.decaySeconds, 0.1f, 120.0f, 3.0f);
        if (std::abs(decay - lastDecay) > 0.000001f)
        {
            for (auto& line : lines)
                line.targetGain = std::pow(0.001f, line.seconds / decay);
            lastDecay = decay;
        }
        targetDamping = bounded(settings.damping, 0.0f, 1.0f, 0.45f);
        targetMix = bounded(settings.mix, 0.0f, 1.0f, 0.35f);
    }

    void process(float& left, float& right) noexcept
    {
        left = std::isfinite(left) ? left : 0.0f;
        right = std::isfinite(right) ? right : 0.0f;
        damping += smoothing * (targetDamping - damping);
        mix += smoothing * (targetMix - mix);

        std::array<float, 4> taps {};
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            line.low += (1.0f - 0.92f * damping) * (line.samples[line.position] - line.low);
            line.gain += smoothing * (line.targetGain - line.gain);
            taps[i] = line.low * line.gain;
        }

        const auto [a, b, c, d] = taps;
        const std::array mixed { a + b + c + d, a - b + c - d, a + b - c - d, a - b - c + d };
        const std::array input { left, right, -left, -right };
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            const auto value = 0.5f * mixed[i] + 0.22f * std::clamp(input[i], -8.0f, 8.0f);
            line.samples[line.position] = std::isfinite(value) ? std::clamp(value, -8.0f, 8.0f) : 0.0f;
            if (++line.position == line.samples.size())
                line.position = 0;
        }

        std::array<float, 2> early {};
        for (std::size_t i = 0; i < reflections.size(); ++i)
        {
            const auto delay = reflectionDelays[i];
            const auto index = earlyPosition >= delay ? earlyPosition - delay
                                                       : earlyHistory.size() + earlyPosition - delay;
            const auto [l, r] = earlyHistory[index];
            const auto direct = reflections[i].direct;
            const auto cross = reflections[i].cross;
            early[0] += direct * l + cross * r;
            early[1] += direct * r + cross * l;
        }
        earlyHistory[earlyPosition] = { std::clamp(left, -8.0f, 8.0f), std::clamp(right, -8.0f, 8.0f) };
        if (++earlyPosition == earlyHistory.size())
            earlyPosition = 0;

        left = (1.0f - mix) * left + mix * (early[0] + 0.45f * (a + b - c - d));
        right = (1.0f - mix) * right + mix * (early[1] + 0.45f * (a - b + c - d));
    }

private:
    static float bounded(float value, float minimum, float maximum, float fallback) noexcept
    {
        return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
    }

    struct Line
    {
        std::vector<float> samples;
        std::size_t position = 0;
        float seconds = 0.0f;
        float low = 0.0f;
        float gain = 0.0f;
        float targetGain = 0.0f;
    };

    struct Reflection
    {
        double seconds;
        float direct;
        float cross;
    };

    inline static constexpr std::array reflections {
        Reflection { 0.007, 0.12f, 0.0f },
        Reflection { 0.011, 0.0f, 0.09f },
        Reflection { 0.017, 0.07f, 0.03f },
        Reflection { 0.023, 0.02f, 0.06f }
    };
    std::array<std::size_t, reflections.size()> reflectionDelays {};
    std::vector<std::array<float, 2>> earlyHistory;
    std::size_t earlyPosition = 0;
    std::array<Line, 4> lines;
    float lastDecay = -1.0f;
    float smoothing = 1.0f;
    float damping = 0.45f;
    float targetDamping = damping;
    float mix = 0.35f;
    float targetMix = mix;
};
}
