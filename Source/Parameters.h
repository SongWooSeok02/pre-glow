#pragma once
#include <tuple>
#include <array>
#include "../DSP/Pitch/Shimmer.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace preglow::parameters
{
inline constexpr auto decay = "decay";
inline constexpr auto damping = "damping";
inline constexpr auto mix = "mix";
inline constexpr std::array crossoverIds { "low_mid_hz", "mid_high_hz" };
inline constexpr std::array crossoverNames { "Low / Mid", "Mid / High" };
inline constexpr std::array bandModIds { "low_mod_depth", "mid_mod_depth", "high_mod_depth" };
inline constexpr std::array bandModNames { "Low Mod Depth", "Mid Mod Depth", "High Mod Depth" };
inline constexpr std::array bandSendIds { "low_shimmer_send", "mid_shimmer_send", "high_shimmer_send" };
inline constexpr std::array bandSendNames { "Low Shimmer Send", "Mid Shimmer Send", "High Shimmer Send" };
inline constexpr std::array bandLevelIds { "low_band_level", "mid_band_level", "high_band_level" };
inline constexpr std::array bandLevelNames { "Low Level", "Mid Level", "High Level" };
inline constexpr std::array bandDampingIds { "low_feedback_damping", "mid_feedback_damping", "high_feedback_damping" };
inline constexpr std::array bandDampingNames { "Low Damping", "Mid Damping", "High Damping" };
inline constexpr std::array bandDecayIds { "low_decay", "mid_decay", "high_decay" };
inline constexpr std::array bandDecayNames { "Low Decay", "Mid Decay", "High Decay" };
inline constexpr auto filterPlacement = "filter_placement";
inline constexpr auto lowCut = "low_cut";
inline const juce::StringArray filterPlacements { "Low Cut Off", "Reverb Input", "Wet Output" };
inline constexpr auto characterB = "character_b";
inline constexpr auto morphX = "morph_x";
inline constexpr auto morphY = "morph_y";
inline constexpr auto characterBlend = "character_blend";
inline constexpr auto character = "character";
inline const juce::StringArray characterNames { "Hall", "Room", "Plate", "Cloud", "Cathedral", "Void" };
inline constexpr auto earlyWidth = "early_width";
inline constexpr auto lateWidth = "late_width";
inline constexpr auto dry = "dry";
inline constexpr auto wet = "wet";
inline constexpr auto freeze = "freeze";
inline constexpr auto infinite = "infinite";
inline constexpr auto bloomAmount = "bloom_amount";
inline constexpr auto bloomAttack = "bloom_attack";
inline constexpr auto bloomRelease = "bloom_release";
inline constexpr auto bloomShape = "bloom_shape";
inline constexpr auto bloomEnabled = "bloom_enabled";
inline constexpr auto swellEnabled = "swell_enabled";
inline constexpr auto swellTime = "swell_time";
inline constexpr auto swellSensitivity = "swell_sensitivity";
inline constexpr auto swellCurve = "swell_curve";
inline constexpr auto stateType = "PreGlowState";
inline constexpr int schemaVersion = 7;
inline constexpr auto shimmerEnabled = "shimmer_enabled";
inline constexpr auto shimmerRouting = "shimmer_routing";
inline const juce::StringArray shimmerRoutes { "Feedback", "Post-Reverb", "Pre-Reverb", "Parallel" };
inline constexpr auto shimmerMix = "shimmer_mix";
inline constexpr auto shimmerFeedback = "shimmer_feedback";
struct VoiceParameter
{
    const char* suffix;
    const char* label;
    float minimum, maximum, step, initial;
};
inline constexpr std::array voiceParameters {
    VoiceParameter { "pitch", "Pitch", -24.0f, 24.0f, 1.0f, 12.0f },
    VoiceParameter { "fine", "Fine (ct)", -100.0f, 100.0f, 1.0f, 0.0f },
    VoiceParameter { "level", "Level", 0.0f, 1.0f, 0.01f, 0.0f },
    VoiceParameter { "pan", "Pan", -1.0f, 1.0f, 0.01f, 0.0f },
    VoiceParameter { "delay", "Delay (ms)", 0.0f, 250.0f, 1.0f, 0.0f },
    VoiceParameter { "send", "Send", 0.0f, 1.0f, 0.01f, 0.5f },
    VoiceParameter { "tone", "Tone (Hz)", 100.0f, 18000.0f, 1.0f, 6000.0f }
};
inline constexpr VoiceParameter voiceEnabled { "enabled", "Enabled", 0, 1, 1, 0 };
inline juce::String voiceId(std::size_t voice, const VoiceParameter& parameter)
{
    return "shimmer_" + juce::String(static_cast<int>(voice + 1)) + "_" + parameter.suffix;
}

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
    for (const auto& [id, name, initial] : { std::tuple { bloomAmount, "Bloom Amount", 75.0f },
                                           std::tuple { bloomShape, "Bloom Shape", 50.0f } })
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 7 }, name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, initial,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    for (const auto& [id, name, minimum, maximum, initial] :
         { std::tuple { bloomAttack, "Bloom Attack", 10.0f, 2000.0f, 250.0f },
           std::tuple { bloomRelease, "Bloom Release", 100.0f, 10000.0f, 1500.0f } })
    {
        juce::NormalisableRange<float> range { minimum, maximum, 1.0f };
        range.setSkewForCentre(initial);
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 7 }, name, range,
            initial, juce::AudioParameterFloatAttributes().withLabel("ms")));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { bloomEnabled, 8 }, "Bloom", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { swellEnabled, 8 }, "Swell", false));
    juce::NormalisableRange<float> swellRange { 10.0f, 4000.0f, 1.0f };
    swellRange.setSkewForCentre(400.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { swellTime, 8 }, "Swell Time", swellRange, 400.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    for (const auto& [id, name] : { std::pair { swellSensitivity, "Swell Sensitivity" }, std::pair { swellCurve, "Swell Curve" } })
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 8 }, name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 50.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));
    for (const auto& [id, name] : { std::pair { shimmerMix, "Shimmer Mix" },
                                    std::pair { shimmerFeedback, "Shimmer Feedback" } })
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 9 }, name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

    for (std::size_t voice = 0; voice < Shimmer::voiceCount; ++voice)
        for (const auto& parameter : voiceParameters)
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { voiceId(voice, parameter), 9 },
                "Shimmer " + juce::String(static_cast<int>(voice + 1)) + " " + parameter.label,
                juce::NormalisableRange<float> { parameter.minimum, parameter.maximum, parameter.step },
                voice == 0 && juce::String(parameter.suffix) == "level" ? 1.0f : parameter.initial));
    for (std::size_t voice = 0; voice < Shimmer::voiceCount; ++voice)
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { voiceId(voice, voiceEnabled), 10 },
            "Shimmer Voice " + juce::String(static_cast<int>(voice + 1)) + " Enabled", voice == 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { shimmerRouting, 11 }, "Shimmer Routing", shimmerRoutes, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { shimmerEnabled, 12 }, "Shimmer Enabled", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { characterB, 13 }, "Character B", characterNames, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { characterBlend, 13 }, "Reverb A/B Blend",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { filterPlacement, 14 },
        "Low Cut Placement", filterPlacements, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { lowCut, 14 }, "Low Cut",
        juce::NormalisableRange<float> { 20.0f, 2000.0f, 1.0f, 0.4f }, 120.0f));
    for (std::size_t band = 0; band < bandDecayIds.size(); ++band)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { bandDecayIds[band], 15 },
            bandDecayNames[band], juce::NormalisableRange<float> { 0.25f, 2.0f, 0.01f }, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { crossoverIds[0], 16 },
        crossoverNames[0], juce::NormalisableRange<float> { 40, 1000, 1, 0.5f }, 250.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { crossoverIds[1], 16 },
        crossoverNames[1], juce::NormalisableRange<float> { 1100, 12000, 1, 0.5f }, 4000.0f));
    for (std::size_t band = 0; band < bandDampingIds.size(); ++band)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { bandDampingIds[band], 17 },
            bandDampingNames[band], juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 0.0f));
    for (std::size_t band = 0; band < bandLevelIds.size(); ++band)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { bandLevelIds[band], 18 },
            bandLevelNames[band], juce::NormalisableRange<float> { 0.0f, 200.0f, 1.0f }, 100.0f));
    for (std::size_t band = 0; band < bandSendIds.size(); ++band)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { bandSendIds[band], 19 },
            bandSendNames[band], juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 100.0f));
    for (std::size_t band = 0; band < bandModIds.size(); ++band)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { bandModIds[band], 20 },
            bandModNames[band], juce::NormalisableRange<float> { 0.0f, 100.0f, 1.0f }, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { morphX, 21 },
        "Morph X (A/B)", juce::NormalisableRange<float> { -1.0f, 1.0f }, -1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { morphY, 21 },
        "Morph Y (Wet)", juce::NormalisableRange<float> { -1.0f, 1.0f }, 0.0f));
    return layout;
}
}
