#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace preglow
{
// One channel of a dual-head granular shifter. Allocate only in prepare().
class PitchShifter
{
public:
    void prepare(double sampleRate)
    {
        window = std::max(4, static_cast<int>(std::round(sampleRate * 0.04)));
        delay.assign(static_cast<std::size_t>(window + 4), 0.0f);
        reset();
    }

    void reset() noexcept
    {
        std::fill(delay.begin(), delay.end(), 0.0f);
        write = 0;
        phase = 0.0;
    }

    void setSemitones(float semitones) noexcept
    {
        const auto ratio = std::exp2(static_cast<double>(std::clamp(semitones, -24.0f, 24.0f)) / 12.0);
        phaseSlope = 1.0 - ratio;
    }

    float process(float input) noexcept
    {
        if (delay.empty())
            return 0.0f;

        delay[write] = input;
        const auto other = phase < 0.5 ? phase + 0.5 : phase - 0.5;
        constexpr double twoPi = 6.283185307179586;
        // Each head wraps at zero gain; the complementary weights sum to one.
        const auto gain = static_cast<float>(0.5 - 0.5 * std::cos(twoPi * phase));
        const auto output = gain * read(phase) + (1.0f - gain) * read(other);

        if (++write == delay.size())
            write = 0;
        phase += phaseSlope / window;
        phase -= std::floor(phase);
        return output;
    }

private:
    float read(double position) const noexcept
    {
        auto index = static_cast<double>(write) - (1.0 + position * window);
        if (index < 0.0)
            index += static_cast<double>(delay.size());
        const auto first = static_cast<std::size_t>(index);
        const auto next = first + 1 == delay.size() ? 0 : first + 1;
        const auto fraction = static_cast<float>(index - static_cast<double>(first));
        return delay[first] + fraction * (delay[next] - delay[first]);
    }

    std::vector<float> delay;
    std::size_t write = 0;
    int window = 4;
    double phase = 0.0;
    double phaseSlope = 0.0;
};
}
