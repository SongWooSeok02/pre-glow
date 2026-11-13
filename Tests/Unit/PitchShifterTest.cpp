#include "../../DSP/Pitch/PitchShifter.h"

#include <iostream>

int main()
{
    constexpr double twoPi = 6.283185307179586;
    // The window spans 16 periods at 400 Hz, isolating the expected pitch
    // from the sidebands of the head crossfade.
    constexpr double inputHz = 400.0;
    for (const auto rate : { 44100, 48000, 88200, 96000 })
    {
        for (const auto semitones : { -24.0f, -12.0f, 0.0f, 7.0f, 12.0f, 24.0f })
        {
            preglow::PitchShifter shifter;
            shifter.setSemitones(semitones);
            shifter.prepare(rate);
            const auto target = inputHz * std::exp2(semitones / 12.0);
            double sine = 0.0, cosine = 0.0, energy = 0.0;
            for (int i = 0; i < rate; ++i)
            {
                const auto sample = shifter.process(static_cast<float>(std::sin(twoPi * inputHz * i / rate)));
                if (!std::isfinite(sample) || std::abs(sample) > 1.00001f)
                    return 1;
                if (i >= rate / 4)
                {
                    sine += sample * std::sin(twoPi * target * i / rate);
                    cosine += sample * std::cos(twoPi * target * i / rate);
                    energy += sample * sample;
                }
            }
            const auto count = rate - rate / 4;
            if (energy < 0.1 * count || 2.0 * (sine * sine + cosine * cosine) < 0.9 * count * energy)
            {
                std::cerr << "Pitch check failed at " << rate << " Hz, " << semitones << " semitones\n";
                return 1;
            }

            shifter.reset();
            for (int i = 0; i < rate / 10; ++i)
                if (shifter.process(0.0f) != 0.0f)
                    return 1;
        }
    }
}
