#include "../../DSP/Reverb/Reverb.h"
#include <iostream>

int main()
{
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        preglow::Reverb early;
        early.prepare(rate);
        early.setTarget({ 3.0f, 0.45f, 1.0f });
        early.reset();
        const auto directSample = static_cast<int>(std::round(rate * 0.007));
        const auto crossSample = static_cast<int>(std::round(rate * 0.011));
        for (int sample = 0; sample <= crossSample; ++sample)
        {
            float left = sample == 0 ? 1.0f : 0.0f;
            float right = 0.0f;
            early.process(left, right);
            const auto expectedLeft = sample == directSample ? 0.12f : 0.0f;
            const auto expectedRight = sample == crossSample ? 0.09f : 0.0f;
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
        diffused.setTarget({ 3.0f, 0.0f, 1.0f });
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
            reverb.setTarget({ decay, 0.0f, 1.0f });
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
        monoReverb.setTarget({ 3.0f, 0.45f, 1.0f });
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

        std::cout << rate << " Hz passed\n";
    }
}
