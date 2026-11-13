#include "../../DSP/Reverb/Reverb.h"
#include <iostream>

int main()
{
    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
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
        std::cout << rate << " Hz passed\n";
    }
}
