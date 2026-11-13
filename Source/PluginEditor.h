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
    std::array<juce::Slider, 3> sliders;
    std::array<juce::Label, 3> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> attachments;
};
}
