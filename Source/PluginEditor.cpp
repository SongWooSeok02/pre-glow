#include "PluginEditor.h"

namespace preglow
{
PluginEditor::PluginEditor(PluginProcessor& owner) : AudioProcessorEditor(owner)
{
    constexpr std::array ids { parameters::decay, parameters::damping, parameters::dry, parameters::wet, parameters::earlyWidth, parameters::lateWidth };
    constexpr std::array names { "Decay", "Damping", "Dry", "Wet", "Early Width", "Late Width" };
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
    character.setTitle("Reverb character");
    character.addItemList(parameters::characterNames, 1);
    addAndMakeVisible(character);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        owner.state, parameters::character, character);
    addAndMakeVisible(freeze);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        owner.state, parameters::freeze, freeze);
    setResizable(true, true);
    setResizeLimits(560, 250, 900, 560);
    setSize(620, 290);
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff151627));
    graphics.setColour(juce::Colour(0xfff0eaff));
    graphics.setFont(juce::FontOptions(30.0f));
    graphics.drawText(JucePlugin_Name, getLocalBounds().removeFromTop(68).reduced(24, 0), juce::Justification::centredLeft);
}

void PluginEditor::resized()
{
    auto controls = getLocalBounds().reduced(16);
    controls.removeFromTop(56);
    auto options = controls.removeFromTop(28);
    character.setBounds(options.removeFromLeft(130));
    options.removeFromLeft(12);
    freeze.setBounds(options.removeFromLeft(90));
    controls.removeFromTop(8);
    const auto width = controls.getWidth() / static_cast<int>(sliders.size());
    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        auto column = controls.removeFromLeft(width).reduced(6);
        labels[i].setBounds(column.removeFromTop(24));
        sliders[i].setBounds(column);
    }
}
}
