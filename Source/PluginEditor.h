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
