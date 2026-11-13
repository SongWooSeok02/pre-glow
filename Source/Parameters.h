#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace preglow::parameters
{
inline constexpr auto decay = "decay";
inline constexpr auto damping = "damping";
inline constexpr auto mix = "mix";
inline constexpr auto character = "character";
inline const juce::StringArray characterNames { "Hall", "Room", "Plate" };
inline constexpr auto stateType = "PreGlowState";
inline constexpr int schemaVersion = 1;

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::NormalisableRange<float> decayRange { 0.1f, 120.0f, 0.01f };
    decayRange.setSkewForCentre(3.0f);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { decay, 1 }, "Decay", decayRange, 3.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { damping, 1 }, "Damping", 0.0f, 1.0f, 0.45f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { mix, 1 }, "Mix", 0.0f, 1.0f, 0.35f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { character, 2 }, "Character", characterNames, 0));
    return layout;
}
}
