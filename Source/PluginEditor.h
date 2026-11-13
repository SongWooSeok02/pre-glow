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
    juce::ComboBox character;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::array<juce::Slider, 6> sliders;
    std::array<juce::Label, 6> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> attachments;
};
}
