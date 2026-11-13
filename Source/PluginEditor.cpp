#include "PluginEditor.h"

namespace preglow
{
PluginEditor::PluginEditor(PluginProcessor& owner) : AudioProcessorEditor(owner)
{
    constexpr std::array ids { parameters::decay, parameters::damping, parameters::mix };
    constexpr std::array names { "Decay", "Damping", "Mix" };
    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        auto& slider = sliders[i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 24);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffaa91ff));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff373850));
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setTitle(names[i]);
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        addAndMakeVisible(slider);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(owner.state, ids[i], slider);
    }
    setResizable(true, true);
    setResizeLimits(360, 220, 900, 560);
    setSize(520, 290);
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff151627));
    graphics.setColour(juce::Colour(0xfff0eaff));
    graphics.setFont(juce::FontOptions(30.0f));
    graphics.drawText("pre-glow", getLocalBounds().removeFromTop(68).reduced(24, 0), juce::Justification::centredLeft);
}

void PluginEditor::resized()
{
    auto controls = getLocalBounds().reduced(16);
    controls.removeFromTop(56);
    const auto width = controls.getWidth() / static_cast<int>(sliders.size());
    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        auto column = controls.removeFromLeft(width).reduced(6);
        labels[i].setBounds(column.removeFromTop(24));
        sliders[i].setBounds(column);
    }
}
}
