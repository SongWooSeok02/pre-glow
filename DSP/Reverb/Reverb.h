#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace preglow
{
enum class ReverbCharacter { hall, room, plate, cloud, cathedral, voidSpace, count };

struct ReverbSettings
{
    float decaySeconds = 3.0f;
    float damping = 0.45f;
    float wet = 0.35f;
    float earlyWidth = 1.0f;
    float lateWidth = 1.0f;
    float dry = 0.65f;
    bool freeze = false;
};

class Reverb
{
public:
    void prepare(double sampleRate, ReverbCharacter character = ReverbCharacter::hall)
    {
        constexpr std::array diffusionSeconds { 0.0031, 0.0047, 0.0061, 0.0083 };
        constexpr std::array delaySeconds { 0.0297, 0.0371, 0.0411, 0.0437 };
        struct Profile
        {
            double delayScale, diffusionScale, reflectionScale;
            float earlyLevel, diffusion, extraDiffusion, dampingScale;
        };
        constexpr std::array profiles {
            Profile { 1.00, 1.00, 1.00, 0.40f, 0.60f, 0.53f, 1.0f }, // Hall
            Profile { 0.55, 1.00, 0.58, 0.48f, 0.40f, 0.49f, 1.0f }, // Room
            Profile { 0.78, 0.46, 0.75, 0.20f, 0.73f, 0.0f, 1.0f }, // Plate
            Profile { 0.92, 1.71, 1.18, 0.25f, 0.78f, 0.64f, 1.45f }, // Cloud
            Profile { 2.23, 1.24, 2.08, 0.90f, 0.57f, 0.0f, 1.2f }, // Cathedral
            Profile { 1.64, 2.35, 1.51, 0.10f, 0.69f, -0.65f, 0.55f } // Void
        };
        static_assert(profiles.size() == static_cast<std::size_t>(ReverbCharacter::count));
        const auto [delayScale, diffusionScale, reflectionScale, level, diffusion, extraDiffusion, profileDamping] =
            profiles[std::min(static_cast<std::size_t>(character), profiles.size() - 1)];
        earlyLevel = level;
        softenReflections = character == ReverbCharacter::hall || character == ReverbCharacter::room;
        reflectionFilter = static_cast<float>(-std::expm1(-1.0 / (0.00035 * sampleRate)));
        dampingScale = profileDamping;
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            line.diffusion.samples.resize(static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * diffusionSeconds[i] * diffusionScale))));
            line.diffusion.coefficient = diffusion;
            line.extraDiffusion.coefficient = extraDiffusion;
            line.extraDiffusion.samples.resize(extraDiffusion == 0.0f ? 0 :
                static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * diffusionSeconds[3 - i] * 1.37))));
            line.samples.assign(static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * delaySeconds[i] * delayScale))), 0.0f);
            line.seconds = static_cast<float>(static_cast<double>(line.samples.size()) / sampleRate);
        }

        for (std::size_t i = 0; i < reflections.size(); ++i)
            reflectionDelays[i] = static_cast<std::size_t>(std::max(1.0, std::round(sampleRate * reflections[i].seconds * reflectionScale)));
        earlyHistory.resize(reflectionDelays.back() + 1);

        lastDecay = -1.0f;

        smoothing = static_cast<float>(-std::expm1(-1.0 / (0.025 * sampleRate)));
        freezeStep = static_cast<float>(1.0 / (0.10 * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        for (auto& line : lines)
        {
            std::fill(line.samples.begin(), line.samples.end(), 0.0f);
            line.position = 0;
            line.diffusion.reset();
            line.extraDiffusion.reset();
            line.low = 0.0f;
            line.gain = line.targetGain;
        }
        std::fill(earlyHistory.begin(), earlyHistory.end(), std::array<float, 2> {});
        earlyPosition = 0;
        damping = targetDamping;
        freeze = targetFreeze ? 1.0f : 0.0f;
        reflectionLow.fill(0.0f);
        dry = targetDry;
        wet = targetWet;
        earlyWidth = targetEarlyWidth;
        lateWidth = targetLateWidth;
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
        targetFreeze = settings.freeze;
        targetDamping = bounded(settings.damping, 0.0f, 1.0f, 0.45f);
        targetEarlyWidth = bounded(settings.earlyWidth, 0.0f, 2.0f, 1.0f);
        targetLateWidth = bounded(settings.lateWidth, 0.0f, 2.0f, 1.0f);
        targetWet = bounded(settings.wet, 0.0f, 1.0f, 0.35f);
        targetDry = bounded(settings.dry, 0.0f, 1.0f, 0.65f);
    }

    void process(float& left, float& right) noexcept
    {
        left = std::isfinite(left) ? left : 0.0f;
        right = std::isfinite(right) ? right : 0.0f;
        damping += smoothing * (targetDamping - damping);
        wet += smoothing * (targetWet - wet);
        dry += smoothing * (targetDry - dry);

        earlyWidth += smoothing * (targetEarlyWidth - earlyWidth);
        lateWidth += smoothing * (targetLateWidth - lateWidth);
        freeze = std::clamp(freeze + (targetFreeze ? freezeStep : -freezeStep), 0.0f, 1.0f);
        const auto feed = 1.0f - freeze;
        const auto dampingAmount = feed * std::min(1.0f, damping * dampingScale);
        std::array<float, 4> taps {};
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            line.low += (1.0f - 0.92f * dampingAmount) * (line.samples[line.position] - line.low);
            line.gain += smoothing * (line.targetGain - line.gain);
            // At full Freeze the normalized Hadamard feedback is lossless.
            taps[i] = line.low * (freeze + feed * line.gain);
        }

        const auto [a, b, c, d] = taps;
        const std::array mixed { a + b + c + d, a - b + c - d, a + b - c - d, a - b - c + d };
        const std::array input { left, right, -left, -right };
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            auto diffused = line.diffusion.process(feed * std::clamp(input[i], -8.0f, 8.0f));
            if (!line.extraDiffusion.samples.empty())
                diffused = line.extraDiffusion.process(diffused);
            const auto value = 0.5f * mixed[i] + 0.22f * feed * diffused;
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
        earlyHistory[earlyPosition] = { feed * std::clamp(left, -8.0f, 8.0f), feed * std::clamp(right, -8.0f, 8.0f) };
        if (++earlyPosition == earlyHistory.size())
            earlyPosition = 0;

        if (softenReflections)
        {
            for (std::size_t channel = 0; channel < early.size(); ++channel)
            {
                reflectionLow[channel] += reflectionFilter * (early[channel] - reflectionLow[channel]);
                early[channel] = reflectionLow[channel];
            }
        }
        auto lateLeft = 0.45f * (a + b - c - d);
        auto lateRight = 0.45f * (a - b + c - d);
        applyWidth(early[0], early[1], earlyWidth);
        applyWidth(lateLeft, lateRight, lateWidth);
        left = dry * left + wet * (earlyLevel * early[0] + lateLeft);
        right = dry * right + wet * (earlyLevel * early[1] + lateRight);
    }

private:
    static void applyWidth(float& left, float& right, float width) noexcept
    {
        const auto mid = 0.5f * (left + right);
        const auto side = 0.5f * (left - right) * width;
        left = mid + side;
        right = mid - side;
    }

    static float bounded(float value, float minimum, float maximum, float fallback) noexcept
    {
        return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
    }

    struct Diffuser
    {
        void reset() noexcept
        {
            std::fill(samples.begin(), samples.end(), 0.0f);
            position = 0;
        }

        float process(float input) noexcept
        {
            const auto delayed = samples[position];
            const auto output = delayed - coefficient * input;
            samples[position] = input + coefficient * output;
            if (++position == samples.size())
                position = 0;
            return output;
        }

        std::vector<float> samples;
        std::size_t position = 0;
        float coefficient = 0.6f;
    };

    struct Line
    {
        std::vector<float> samples;
        std::size_t position = 0;
        float seconds = 0.0f;
        float low = 0.0f;
        float gain = 0.0f;
        float targetGain = 0.0f;
        Diffuser diffusion;
        Diffuser extraDiffusion;
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
    float dampingScale = 1.0f;
    float earlyWidth = 1.0f;
    float lateWidth = 1.0f;
    float targetEarlyWidth = 1.0f;
    float targetLateWidth = 1.0f;
    float earlyLevel = 1.0f;
    float lastDecay = -1.0f;
    float smoothing = 1.0f;
    float freeze = 0.0f;
    float freezeStep = 1.0f;
    bool targetFreeze = false;
    float damping = 0.45f;
    float targetDamping = damping;
    bool softenReflections = false;
    float reflectionFilter = 1.0f;
    std::array<float, 2> reflectionLow {};
    float dry = 0.65f;
    float wet = 0.35f;
    float targetDry = dry;
    float targetWet = wet;
};
}
