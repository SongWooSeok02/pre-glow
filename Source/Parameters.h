#pragma once
#include <tuple>
#include <juce_audio_processors/juce_audio_processors.h>

namespace preglow::parameters
{
inline constexpr auto decay = "decay";
inline constexpr auto damping = "damping";
inline constexpr auto mix = "mix";
inline constexpr auto character = "character";
inline const juce::StringArray characterNames { "Hall", "Room", "Plate", "Cloud", "Cathedral", "Void" };
inline constexpr auto earlyWidth = "early_width";
inline constexpr auto lateWidth = "late_width";
inline constexpr auto dry = "dry";
inline constexpr auto wet = "wet";
inline constexpr auto freeze = "freeze";
inline constexpr auto infinite = "infinite";
inline constexpr auto stateType = "PreGlowState";
inline constexpr int schemaVersion = 2;

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::NormalisableRange<float> decayRange { 0.1f, 120.0f, 0.01f };
    decayRange.setSkewForCentre(3.0f);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { decay, 1 }, "Decay", decayRange, 3.0f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { damping, 1 }, "Damping", 0.0f, 1.0f, 0.45f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { mix, 1 }, "Mix (Legacy)", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { character, 2 }, "Character", characterNames, 0));
    for (const auto& [id, name] : { std::pair { earlyWidth, "Early Width" }, std::pair { lateWidth, "Late Width" } })
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 3 }, name,
            juce::NormalisableRange<float> { 0.0f, 200.0f, 1.0f }, 100.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    for (const auto& [id, name, initial] : { std::tuple { dry, "Dry", 65.0f }, std::tuple { wet, "Wet", 35.0f } })
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 4 }, name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f }, initial,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { freeze, 5 }, "Freeze", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { infinite, 6 }, "Infinite Decay", false));
    return layout;
}
}
