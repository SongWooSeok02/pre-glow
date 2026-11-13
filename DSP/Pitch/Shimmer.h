#pragma once

#include "PitchShifter.h"
#include <array>

namespace preglow
{
class Shimmer
{
public:
    struct Voice
    {
        float semitones = 12.0f;
        float cents = 0.0f;
        float level = 0.0f;
        float pan = 0.0f;
        float delayMs = 0.0f;
        float feedbackSend = 0.0f;
        float toneHz = 6000.0f;
    };

    struct Output
    {
        std::array<float, 2> wet {};
        std::array<float, 2> feedback {};
    };

    static constexpr std::size_t voiceCount = 4;

    void prepare(double sampleRate)
    {
        rate = sampleRate;
        smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.025 * rate)));
        for (auto& voice : voices)
        {
            voice.pitch.prepare(rate);
            voice.delay.assign(static_cast<std::size_t>(std::ceil(rate * 0.25)) + 2, 0.0f);
        }
        reset();
    }

    void reset() noexcept
    {
        for (auto& voice : voices)
        {
            voice.pitch.reset();
            std::fill(voice.delay.begin(), voice.delay.end(), 0.0f);
            voice.write = 0;
            voice.low = 0.0f;
            voice.current = voice.target;
        }
    }

    void setVoices(const std::array<Voice, voiceCount>& settings) noexcept
    {
        for (std::size_t i = 0; i < voices.size(); ++i)
        {
            const auto& setting = settings[i];
            auto& voice = voices[i];
            voice.pitch.setSemitones(setting.semitones + setting.cents * 0.01f);
            const auto pan = std::clamp(setting.pan, -1.0f, 1.0f);
            const auto level = std::clamp(setting.level, 0.0f, 1.0f);
            const auto cutoff = std::clamp(static_cast<double>(setting.toneHz), 20.0, 0.45 * rate);
            voice.target = {
                level * std::sqrt(0.5f * (1.0f - pan)),
                level * std::sqrt(0.5f * (1.0f + pan)),
                std::clamp(setting.feedbackSend, 0.0f, 1.0f),
                static_cast<float>(std::clamp(setting.delayMs, 0.0f, 250.0f) * rate * 0.001),
                static_cast<float>(1.0 - std::exp(-6.283185307179586 * cutoff / rate))
            };
        }
    }

    Output process(float left, float right) noexcept
    {
        Output output;
        if (voices.front().delay.empty())
            return output;

        const auto input = 0.5f * (left + right);
        for (auto& voice : voices)
        {
            for (std::size_t i = 0; i < voice.current.size(); ++i)
                voice.current[i] += smoothing * (voice.target[i] - voice.current[i]);

            const auto [leftGain, rightGain, send, samples, tone] = voice.current;
            const auto pitched = voice.pitch.process(input);
            voice.low += tone * (pitched - voice.low);
            voice.delay[voice.write] = voice.low;
            auto read = static_cast<double>(voice.write) - samples;
            if (read < 0.0)
                read += static_cast<double>(voice.delay.size());
            const auto first = static_cast<std::size_t>(read);
            const auto next = (first + 1) % voice.delay.size();
            const auto sample = voice.delay[first]
                + static_cast<float>(read - static_cast<double>(first)) * (voice.delay[next] - voice.delay[first]);
            voice.write = (voice.write + 1) % voice.delay.size();

            const std::array<float, 2> gains { leftGain, rightGain };
            for (std::size_t channel = 0; channel < gains.size(); ++channel)
            {
                const auto signal = sample * gains[channel];
                output.wet[channel] += signal;
                // Keep the sum of four full-level sends at or below unity.
                output.feedback[channel] += signal * send / static_cast<float>(voiceCount);
            }
        }
        return output;
    }

private:
    struct VoiceState
    {
        PitchShifter pitch;
        std::vector<float> delay;
        std::size_t write = 0;
        float low = 0.0f;
        std::array<float, 5> current {};
        std::array<float, 5> target {};
    };

    std::array<VoiceState, voiceCount> voices;
    double rate = 48000.0;
    float smoothing = 1.0f;
};
}
