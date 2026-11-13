#include "../../DSP/Reverb/Reverb.h"
#include <iostream>

int main()
{
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        preglow::Reverb early;
        early.prepare(rate, preglow::ReverbCharacter::plate);
        early.setTarget({ 3.0f, 0.45f, 1.0f, 1.0f, 1.0f, 0.0f });
        early.reset();
        const auto directSample = static_cast<int>(std::round(rate * 0.007 * 0.75));
        const auto crossSample = static_cast<int>(std::round(rate * 0.011 * 0.75));
        for (int sample = 0; sample <= crossSample; ++sample)
        {
            float left = sample == 0 ? 1.0f : 0.0f;
            float right = 0.0f;
            early.process(left, right);
            const auto expectedLeft = sample == directSample ? 0.024f : 0.0f;
            const auto expectedRight = sample == crossSample ? 0.018f : 0.0f;
            if (!(std::abs(left - expectedLeft) < 1.0e-6f && std::abs(right - expectedRight) < 1.0e-6f))
                return 4;
        }
        early.reset();
        for (int sample = 0; sample < static_cast<int>(rate * 0.03); ++sample)
        {
            float left = 0.0f;
            float right = 0.0f;
            early.process(left, right);
            if (left != 0.0f || right != 0.0f)
                return 5;
        }

        preglow::Reverb diffused;
        diffused.prepare(rate);
        diffused.setTarget({ 3.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f });
        diffused.reset();
        // The first FDN line should produce a second arrival before any
        // other line or feedback path can contribute to this window.
        double diffusionEnergy = 0.0;
        for (int sample = 0; sample < static_cast<int>(rate * 0.035); ++sample)
        {
            float left = sample == 0 ? 1.0f : 0.0f;
            float right = 0.0f;
            diffused.process(left, right);
            if (sample >= static_cast<int>(rate * 0.031))
                diffusionEnergy += left * left + right * right;
        }
        if (!(diffusionEnergy > 0.00001))
            return 6;

        for (float decay : { 0.1f, 3.0f, 120.0f })
        {
            preglow::Reverb reverb;
            reverb.prepare(rate);
            reverb.setTarget({ decay, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f });
            reverb.reset();
            double tailEnergy = 0.0;
            double sideEnergy = 0.0;
            for (int sample = 0; sample < static_cast<int>(rate * 4); ++sample)
            {
                float left = sample == 0 ? 1.0f : 0.0f;
                float right = 0.0f;
                reverb.process(left, right);
                if (!std::isfinite(left) || !std::isfinite(right) || std::abs(left) > 1.0f || std::abs(right) > 1.0f)
                    return 1;
                if (sample > static_cast<int>(rate * 0.02))
                {
                    tailEnergy += left * left + right * right;
                    sideEnergy += (left - right) * (left - right);
                }
            }
            if (tailEnergy < 0.00001 || sideEnergy < 0.00001)
                return 2;
        }
        preglow::Reverb monoReverb;
        monoReverb.prepare(rate);
        monoReverb.setTarget({ 3.0f, 0.45f, 1.0f, 1.0f, 1.0f, 0.0f });
        monoReverb.reset();
        double monoSideEnergy = 0.0;
        for (int sample = 0; sample < static_cast<int>(rate); ++sample)
        {
            float left = sample == 0 ? 1.0f : 0.0f;
            float right = left;
            monoReverb.process(left, right);
            const auto side = left - right;
            monoSideEnergy += side * side;
        }
        if (!std::isfinite(monoSideEnergy) || monoSideEnergy < 0.00001)
            return 3;

        for (auto character : { preglow::ReverbCharacter::room, preglow::ReverbCharacter::cathedral })
        {
            preglow::Reverb held;
            held.prepare(rate, character);
            preglow::ReverbSettings settings { 2.0f, 0.35f, 1.0f, 1.0f, 1.0f, 0.0f };
            held.setTarget(settings);
            held.reset();
            const auto render = [rate](auto& engine, double seconds, float input)
            {
                double energy = 0.0;
                for (int i = 0; i < static_cast<int>(rate * seconds); ++i)
                {
                    auto left = i % 2 == 0 ? input : -input;
                    auto right = input;
                    engine.process(left, right);
                    if (!std::isfinite(left) || !std::isfinite(right)
                        || std::abs(left) > 2.0f || std::abs(right) > 2.0f)
                        return -1.0;
                    energy += left * left + right * right;
                }
                return energy;
            };

            if (render(held, 0.25, 0.5f) <= 0.0)
                return 7;
            for (int transition = 0; transition < 3; ++transition)
            {
                settings.freeze = true;
                held.setTarget(settings);
                if (render(held, 0.25, 0.0f) <= 0.0)
                    return 8;
                auto reference = held;
                double firstEnergy = 0.0;
                double lastEnergy = 0.0;
                for (int second = 0; second < 5; ++second)
                {
                    lastEnergy = render(held, 1.0, 1.0f);
                    if (lastEnergy != render(reference, 1.0, 0.0f) || lastEnergy <= 0.0)
                        return 9;
                    if (second == 0)
                        firstEnergy = lastEnergy;
                }
                if (lastEnergy < firstEnergy * 0.5 || lastEnergy > firstEnergy * 2.0)
                    return 10;
                settings.freeze = false;
                held.setTarget(settings);
                const auto releaseEnergy = render(held, 3.0, 0.0f);
                const auto remaining = render(held, 1.0, 0.0f);
                if (releaseEnergy <= 0.0 || remaining < 0.0 || remaining > lastEnergy * 0.001)
                    return 11;
                if (render(held, 0.25, 0.5f) <= 0.0)
                    return 12;
            }
        }

        std::cout << rate << " Hz passed\n";
    }
}
