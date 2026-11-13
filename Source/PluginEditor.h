#pragma once
#include "PluginProcessor.h"

namespace preglow
{
class PluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(PluginProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void updateSections();
    juce::TextButton shimmerPanel { "Shimmer" };
    std::array<juce::ToggleButton, Shimmer::voiceCount> voiceToggles;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, Shimmer::voiceCount> voiceToggleAttachments;
    juce::Component voiceContent;
    std::array<juce::GroupComponent, Shimmer::voiceCount> voiceCards;
    std::array<juce::TextButton, Shimmer::voiceCount> voiceDetails;
    juce::Viewport voiceViewport;
    std::array<juce::Slider, 2> shimmerGlobals;
    std::array<juce::Label, 2> shimmerGlobalLabels;
    std::array<juce::Slider, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceSliders;
    std::array<juce::Label, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceLabels;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 2> shimmerAttachments;
    std::array<std::unique_ptr<SliderAttachment>, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceAttachments;
    juce::ToggleButton bloom { "Bloom" }, swell { "Swell" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bloomAttachment, swellAttachment;
    juce::ToggleButton infinite { "Infinite Decay" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> infiniteAttachment;
    juce::ToggleButton freeze { "Freeze" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    juce::ComboBox character;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::array<juce::Slider, 13> sliders;
    std::array<juce::Label, 13> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 13> attachments;
};
}
