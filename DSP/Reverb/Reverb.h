#pragma once
#include "../Pitch/Shimmer.h"

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
    bool infinite = false;
    float bloomAmount = 0.0f;
    float bloomAttackMs = 250.0f;
    float bloomReleaseMs = 1500.0f;
    float bloomShape = 0.5f;
    bool swell = false;
    float swellTimeMs = 400.0f;
    float swellSensitivity = 0.5f;
    float swellCurve = 0.5f;
};

class Reverb
{
public:
    static constexpr float infiniteDecaySeconds = 600.0f;

    void setShimmer(const std::array<Shimmer::Voice, Shimmer::voiceCount>& voices, float mix, float feedback, bool pre = false, bool parallel = false) noexcept
    {
        shimmer.setVoices(voices);
        targetShimmerMix = mix;
        targetShimmerFeedback = feedback;
        targetPreRoute = pre ? 1.0f : 0.0f;
        targetParallelRoute = parallel ? 1.0f : 0.0f;
        preShimmer.setVoices(voices);
    }

    void setBandCrossovers(float lowHz, float highHz) noexcept
    {
        const std::array frequencies { lowHz, highHz };
        if (frequencies == crossoverHz)
            return;
        crossoverHz = frequencies;
        targetLowSplit = -std::expm1(-6.2831853f * lowHz / filterSampleRate);
        targetHighSplit = -std::expm1(-6.2831853f * highHz / filterSampleRate);
    }

    void setBandSend(const std::array<float, 3>& sends) noexcept { targetBandSend = sends; }

    void setBandLevel(const std::array<float, 3>& levels) noexcept { targetBandLevel = levels; }

    void setBandDamping(const std::array<float, 3>& amounts) noexcept { targetBandDamping = amounts; }

    void setBandDecay(const std::array<float, 3>& amounts) noexcept
    {
        if (amounts == cachedBandDecay && std::abs(lastDecay - cachedBandBaseDecay) < 0.000001f)
            return;
        cachedBandDecay = amounts;
        cachedBandBaseDecay = lastDecay;
        for (auto& line : lines)
            for (std::size_t band = 0; band < amounts.size(); ++band)
                line.targetBandGain[band] = std::pow(line.targetGain, 1.0f / amounts[band]);
    }

    void setLowCut(int placement, float hz) noexcept
    {
        targetInputCut = placement == 1 ? 1.0f : 0.0f;
        targetOutputCut = placement == 2 ? 1.0f : 0.0f;
        targetCutCoefficient = -std::expm1(-6.2831853f * hz / filterSampleRate);
    }

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

        lastDecay = cachedBandBaseDecay = -1.0f;

        filterSampleRate = static_cast<float>(sampleRate);
        crossoverHz = {};
        setBandCrossovers(250.0f, 4000.0f);
        smoothing = static_cast<float>(-std::expm1(-1.0 / (0.025 * sampleRate)));
        samplesPerMs = static_cast<float>(sampleRate * 0.001);
        swellGainSmoothing = 1.0f / (1.0f + 5.0f * samplesPerMs);
        swellDetectorRelease = 1.0f / (1.0f + 40.0f * samplesPerMs);
        shimmer.prepare(sampleRate);
        preShimmer.prepare(sampleRate);
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
            line.bandGain = line.targetBandGain;
            line.lowBand = line.belowHigh = 0.0f;
        }
        std::fill(earlyHistory.begin(), earlyHistory.end(), std::array<float, 2> {});
        earlyPosition = 0;
        damping = targetDamping;
        bloomPeak = bloomEnvelope = bloomContour = 0.0f;
        swellPeak = swellProgress = 0.0f;
        swellTriggered = swellInputActive = false;
        swellMix = targetSwellMix;
        swellGain = targetFreeze ? 1.0f : 1.0f - swellMix;
        swellCurve = targetSwellCurve;
        bloomAmount = targetBloomAmount;
        bloomShape = targetBloomShape;
        shimmer.reset();
        preShimmer.reset();
        bandDamping = targetBandDamping;
        bandLevel = targetBandLevel;
        bandSend = targetBandSend;
        shimmerSendFilters = {};
        wetLow.fill(0.0f);
        wetBelowHigh.fill(0.0f);
        lowSplit = targetLowSplit;
        highSplit = targetHighSplit;
        inputLow.fill(0.0f);
        outputLow.fill(0.0f);
        inputCut = targetInputCut;
        outputCut = targetOutputCut;
        cutCoefficient = targetCutCoefficient;
        preRoute = targetPreRoute;
        parallelRoute = targetParallelRoute;
        shimmerMix = targetShimmerMix;
        shimmerFeedback = targetShimmerFeedback;
        freeze = targetFreeze ? 1.0f : 0.0f;
        reflectionLow.fill(0.0f);
        dry = targetDry;
        wet = targetWet;
        earlyWidth = targetEarlyWidth;
        lateWidth = targetLateWidth;
    }

    void setTarget(ReverbSettings settings) noexcept
    {
        // Keep a small feedback loss: Infinite still injects live input.
        const auto decay = settings.infinite ? infiniteDecaySeconds
                                             : bounded(settings.decaySeconds, 0.1f, 120.0f, 3.0f);
        if (std::abs(decay - lastDecay) > 0.000001f)
        {
            for (auto& line : lines)
            {
                line.targetGain = std::pow(0.001f, line.seconds / decay);
                line.targetBandGain.fill(line.targetGain);
            }
            lastDecay = decay;
        }
        targetFreeze = settings.freeze;
        targetBloomAmount = bounded(settings.bloomAmount, 0.0f, 1.0f, 0.0f);
        targetBloomShape = bounded(settings.bloomShape, 0.0f, 1.0f, 0.5f);
        bloomAttack = 1.0f / (1.0f + samplesPerMs * bounded(settings.bloomAttackMs, 10.0f, 2000.0f, 250.0f));
        bloomRelease = 1.0f / (1.0f + samplesPerMs * bounded(settings.bloomReleaseMs, 100.0f, 10000.0f, 1500.0f));
        targetSwellMix = settings.swell ? 1.0f : 0.0f;
        targetSwellCurve = bounded(settings.swellCurve, 0.0f, 1.0f, 0.5f);
        const auto sensitivity = bounded(settings.swellSensitivity, 0.0f, 1.0f, 0.5f);
        swellThreshold = 0.001f + 0.099f * (1.0f - sensitivity) * (1.0f - sensitivity);
        swellStep = 1.0f / (samplesPerMs * bounded(settings.swellTimeMs, 10.0f, 4000.0f, 400.0f));
        targetDamping = bounded(settings.damping, 0.0f, 1.0f, 0.45f);
        targetEarlyWidth = bounded(settings.earlyWidth, 0.0f, 2.0f, 1.0f);
        targetLateWidth = bounded(settings.lateWidth, 0.0f, 2.0f, 1.0f);
        targetWet = bounded(settings.wet, 0.0f, 1.0f, 0.35f);
        targetDry = bounded(settings.dry, 0.0f, 1.0f, 0.65f);
    }

    struct Output { std::array<float, 2> dry, wet; };

    void process(float& left, float& right) noexcept
    {
        const auto output = processSources(left, right);
        left = output.dry[0] + output.wet[0];
        right = output.dry[1] + output.wet[1];
    }

    Output processSources(float left, float right) noexcept
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
        bloomAmount += smoothing * (targetBloomAmount - bloomAmount);
        bloomShape += smoothing * (targetBloomShape - bloomShape);
        const auto level = std::min(1.0f, 4.0f * std::max(std::abs(left), std::abs(right)));
        // Charge on new input, then let the bloom fade with its own release.
        if (targetFreeze)
        {
            bloomPeak = 0.0f;
            bloomEnvelope = 1.0f;
        }
        else if (targetBloomAmount == 0.0f)
            bloomPeak = bloomEnvelope = 0.0f;
        else
        {
            bloomPeak = std::max(level, bloomPeak * (1.0f - bloomRelease));
            bloomEnvelope += bloomAttack * (bloomPeak - bloomEnvelope);
        }
        const auto contour = bloomEnvelope * (1.0f - bloomShape + bloomShape * bloomEnvelope);
        bloomContour += smoothing * (contour - bloomContour);
        const auto bloom = feed * bloomAmount;
        const auto bloomEarlyGain = 1.0f - bloom;
        const auto bloomLateGain = bloomEarlyGain + bloom * bloomContour;
        swellMix += smoothing * (targetSwellMix - swellMix);
        swellCurve += smoothing * (targetSwellCurve - swellCurve);
        if (targetFreeze || targetSwellMix == 0.0f)
        {
            swellPeak = 0.0f;
            swellInputActive = swellTriggered = false;
            // Releasing Freeze should not close the gate on the held tail.
            swellProgress = targetFreeze ? 1.0f : 0.0f;
        }
        else
        {
            swellPeak = std::max(std::max(std::abs(left), std::abs(right)),
                                 swellPeak * (1.0f - swellDetectorRelease));
            if (swellInputActive && swellPeak < swellThreshold * 0.5f)
                swellInputActive = false;
            if (!swellInputActive && swellPeak >= swellThreshold)
            {
                swellInputActive = swellTriggered = true;
                swellProgress = 0.0f;
            }
            if (swellTriggered)
                swellProgress = std::min(1.0f, swellProgress + swellStep);
        }
        const auto swellFade = swellProgress * (1.0f - swellCurve + swellCurve * swellProgress);
        const auto swellTarget = 1.0f - swellMix * (1.0f - swellFade);
        swellGain += swellGainSmoothing * (swellTarget - swellGain);
        const auto swellWetGain = 1.0f - feed * (1.0f - swellGain);
        // Share a single gain per wet path, so the two envelopes cannot compound.
        const auto earlyGain = std::min(bloomEarlyGain, swellWetGain);
        const auto lateGain = std::min(bloomLateGain, swellWetGain);
        const auto dampingAmount = feed * std::min(1.0f, damping * dampingScale);
        lowSplit += smoothing * (targetLowSplit - lowSplit);
        highSplit += smoothing * (targetHighSplit - highSplit);
        for (std::size_t band = 0; band < bandDamping.size(); ++band)
        {
            bandDamping[band] += smoothing * (targetBandDamping[band] - bandDamping[band]);
            bandSend[band] += smoothing * (targetBandSend[band] - bandSend[band]);
            bandLevel[band] += smoothing * (targetBandLevel[band] - bandLevel[band]);
        }
        std::array<float, 4> taps {};
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            line.low += (1.0f - 0.92f * dampingAmount) * (line.samples[line.position] - line.low);
            line.gain += smoothing * (line.targetGain - line.gain);
            // At full Freeze the normalized Hadamard feedback is lossless.
            line.lowBand += lowSplit * (line.low - line.lowBand);
            line.belowHigh += highSplit * (line.low - line.belowHigh);
            for (std::size_t band = 0; band < line.bandGain.size(); ++band)
                line.bandGain[band] += smoothing * (line.targetBandGain[band] - line.bandGain[band]);

            const auto mid = line.belowHigh - line.lowBand;
            const auto high = line.low - line.belowHigh;
            const auto correction = line.lowBand * (line.bandGain[0] * (1.0f - bandDamping[0]) - line.gain)
                                  + mid * (line.bandGain[1] * (1.0f - bandDamping[1]) - line.gain)
                                  + high * (line.bandGain[2] * (1.0f - bandDamping[2]) - line.gain);
            taps[i] = line.low * (freeze + feed * line.gain) + feed * correction;
        }

        const auto [a, b, c, d] = taps;
        shimmerMix += smoothing * (targetShimmerMix - shimmerMix);
        shimmerFeedback += smoothing * (targetShimmerFeedback - shimmerFeedback);
        const auto lateLeftRaw = 0.45f * (a + b - c - d);
        const auto lateRightRaw = 0.45f * (a - b + c - d);
        preRoute += smoothing * (targetPreRoute - preRoute);
        if (std::abs(preRoute - targetPreRoute) < 0.00001f)
            preRoute = targetPreRoute;
        parallelRoute += smoothing * (targetParallelRoute - parallelRoute);
        if (std::abs(parallelRoute - targetParallelRoute) < 0.00001f)
            parallelRoute = targetParallelRoute;
        if (targetShimmerMix == 0.0f && shimmerMix < 0.00001f)
            shimmerMix = 0.0f;

        const auto preMix = shimmerMix * preRoute;
        const auto parallelMix = shimmerMix * parallelRoute;
        const auto lateMix = shimmerMix * std::max(0.0f, 1.0f - preRoute - parallelRoute);
        const auto shifted = processShimmer(shimmer, lateShimmerRunning, lateMix, filterShimmerSend(0, 0.5f * (lateLeftRaw + lateRightRaw)));
        const auto pre = processShimmer(preShimmer, preShimmerRunning, preMix + parallelMix,
                                       filterShimmerSend(1, 0.5f * (feed * left + feed * right)));
        inputCut += smoothing * (targetInputCut - inputCut);
        outputCut += smoothing * (targetOutputCut - outputCut);
        cutCoefficient += smoothing * (targetCutCoefficient - cutCoefficient);
        const auto reverbLeft = applyLowCut(left + preMix * pre.wet[0], inputLow[0], inputCut);
        const auto reverbRight = applyLowCut(right + preMix * pre.wet[1], inputLow[1], inputCut);
        // Reserve part of the normal loop's loss for the pitch-shifted return.
        // This preserves the original feedback coefficient at every setting.
        float loopGain = 0.0f;
        for (const auto& line : lines)
            loopGain = std::max({ loopGain, line.gain, line.bandGain[0], line.bandGain[1], line.bandGain[2] });
        const auto feedbackGain = feed * lateMix * shimmerFeedback
            * std::min(0.25f, 2.0f * (1.0f - loopGain));
        const auto [sendLeft, sendRight] = shifted.feedback;
        const std::array shimmerReturn { sendLeft, sendRight, -sendLeft, -sendRight };
        const std::array mixed { a + b + c + d, a - b + c - d, a + b - c - d, a - b - c + d };
        const std::array input { reverbLeft, reverbRight, -reverbLeft, -reverbRight };
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            auto& line = lines[i];
            auto diffused = line.diffusion.process(feed * std::clamp(input[i], -8.0f, 8.0f));
            if (!line.extraDiffusion.samples.empty())
                diffused = line.extraDiffusion.process(diffused);
            const auto value = 0.5f * mixed[i]
                + feedbackGain * shimmerReturn[i] + 0.22f * feed * diffused;
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
        earlyHistory[earlyPosition] = { feed * std::clamp(reverbLeft, -8.0f, 8.0f), feed * std::clamp(reverbRight, -8.0f, 8.0f) };
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
        auto lateLeft = lateLeftRaw + lateMix * shifted.wet[0];
        auto lateRight = lateRightRaw + lateMix * shifted.wet[1];
        applyWidth(early[0], early[1], earlyWidth);
        applyWidth(lateLeft, lateRight, lateWidth);
        const std::array wetSignal {
            earlyGain * earlyLevel * early[0] + lateGain * lateLeft + feed * parallelMix * pre.wet[0],
            earlyGain * earlyLevel * early[1] + lateGain * lateRight + feed * parallelMix * pre.wet[1]
        };
        Output output { { dry * left, dry * right }, {} };
        for (std::size_t channel = 0; channel < wetSignal.size(); ++channel)
        {
            const auto sample = applyLowCut(wetSignal[channel], outputLow[channel], outputCut);
            auto& low = wetLow[channel];
            auto& belowHigh = wetBelowHigh[channel];
            low += lowSplit * (sample - low);
            belowHigh += highSplit * (sample - belowHigh);
            output.wet[channel] = wet * (sample + low * (bandLevel[0] - 1.0f)
                + (belowHigh - low) * (bandLevel[1] - 1.0f)
                + (sample - belowHigh) * (bandLevel[2] - 1.0f));
        }
        return output;
    }

private:
    float applyLowCut(float sample, float& low, float amount) const noexcept
    {
        low += cutCoefficient * (sample - low);
        return sample - amount * low;
    }

    std::array<float, 2> inputLow {}, outputLow {};
    std::array<float, 2> wetLow {}, wetBelowHigh {};
    std::array<float, 3> bandLevel { 1, 1, 1 }, targetBandLevel { 1, 1, 1 };
    std::array<float, 3> bandDamping {}, targetBandDamping {};
    std::array<float, 2> crossoverHz {};
    float targetLowSplit = 0.0f, targetHighSplit = 0.0f;
    float lowSplit = 0.0f, highSplit = 0.0f;
    float filterSampleRate = 48000.0f;
    float cutCoefficient = 0.0f, targetCutCoefficient = 0.0f;
    float inputCut = 0.0f, targetInputCut = 0.0f;
    float outputCut = 0.0f, targetOutputCut = 0.0f;

    float filterShimmerSend(std::size_t path, float sample) noexcept
    {
        auto& [low, belowHigh] = shimmerSendFilters[path];
        low += lowSplit * (sample - low);
        belowHigh += highSplit * (sample - belowHigh);
        return sample * bandSend[2] + low * (bandSend[0] - bandSend[1])
             + belowHigh * (bandSend[1] - bandSend[2]);
    }

    std::array<std::array<float, 2>, 2> shimmerSendFilters {};
    std::array<float, 3> bandSend { 1, 1, 1 }, targetBandSend { 1, 1, 1 };

    static Shimmer::Output processShimmer(Shimmer& processor, bool& running, float mix,
                                         float input) noexcept
    {
        if (mix > 0.0f)
        {
            running = true;
            return processor.process(input, input);
        }
        if (running)
        {
            processor.reset();
            running = false;
        }
        return {};
    }

    Shimmer shimmer, preShimmer;
    bool lateShimmerRunning = false, preShimmerRunning = false;
    float preRoute = 0.0f, targetPreRoute = 0.0f;
    float parallelRoute = 0.0f, targetParallelRoute = 0.0f;
    float shimmerMix = 0.0f, shimmerFeedback = 0.0f;
    float targetShimmerMix = 0.0f, targetShimmerFeedback = 0.0f;

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
        float lowBand = 0.0f, belowHigh = 0.0f;
        std::array<float, 3> bandGain {}, targetBandGain {};
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
    std::array<float, 3> cachedBandDecay {};
    float cachedBandBaseDecay = -1.0f;
    float lastDecay = -1.0f;
    float smoothing = 1.0f;
    float swellGain = 1.0f;
    float swellGainSmoothing = 1.0f;
    float swellPeak = 0.0f;
    float swellDetectorRelease = 1.0f;
    float swellProgress = 0.0f;
    float swellMix = 0.0f;
    float targetSwellMix = 0.0f;
    float swellStep = 1.0f;
    float swellThreshold = 0.02575f;
    float swellCurve = 0.5f;
    float targetSwellCurve = 0.5f;
    bool swellTriggered = false;
    bool swellInputActive = false;
    float samplesPerMs = 48.0f;
    float bloomPeak = 0.0f;
    float bloomContour = 0.0f;
    float bloomEnvelope = 0.0f;
    float bloomAmount = 0.0f;
    float targetBloomAmount = 0.0f;
    float bloomShape = 0.5f;
    float targetBloomShape = 0.5f;
    float bloomAttack = 1.0f;
    float bloomRelease = 1.0f;
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
