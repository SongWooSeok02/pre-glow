#include "PluginEditor.h"

namespace preglow
{
namespace
{
constexpr int compactHeight = 250;
constexpr int sectionHeight = 136;
constexpr std::size_t bloomStart = 6;
constexpr std::size_t swellStart = 10;
}
PluginEditor::PluginEditor(PluginProcessor& owner) : AudioProcessorEditor(owner)
{
    constexpr std::array ids { parameters::decay, parameters::damping, parameters::dry, parameters::wet, parameters::earlyWidth, parameters::lateWidth,
                               parameters::bloomAmount, parameters::bloomAttack, parameters::bloomRelease, parameters::bloomShape,
                               parameters::swellTime, parameters::swellSensitivity, parameters::swellCurve };
    constexpr std::array names { "Decay", "Damping", "Dry", "Wet", "Early Width", "Late Width",
                                 "Bloom Amount", "Bloom Attack", "Bloom Release", "Bloom Shape",
                                 "Swell Time", "Swell Sensitivity", "Swell Curve" };
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
    addAndMakeVisible(infinite);
    infiniteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        owner.state, parameters::infinite, infinite);
    addAndMakeVisible(bloom);
    addAndMakeVisible(swell);
    bloomAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        owner.state, parameters::bloomEnabled, bloom);
    swellAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        owner.state, parameters::swellEnabled, swell);
    bloom.onClick = swell.onClick = [this] { updateSections(); };
    setSize(620, compactHeight);
    updateSections();
}

void PluginEditor::updateSections()
{
    const auto bloomOpen = bloom.getToggleState();
    const auto swellOpen = swell.getToggleState();
    for (std::size_t i = bloomStart; i < sliders.size(); ++i)
    {
        const auto visible = i < swellStart ? bloomOpen : swellOpen;
        labels[i].setVisible(visible);
        sliders[i].setVisible(visible);
    }
    setSize(getWidth(), compactHeight + sectionHeight * (static_cast<int>(bloomOpen) + static_cast<int>(swellOpen)));
    resized();
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
    infinite.setBounds(options.removeFromLeft(140));
    bloom.setBounds(options.removeFromLeft(85));
    swell.setBounds(options.removeFromLeft(85));
    controls.removeFromTop(8);
    const auto layoutRow = [&](juce::Rectangle<int> row, std::size_t start, std::size_t count)
    {
        const auto width = row.getWidth() / static_cast<int>(count);
        for (auto i = start; i < start + count; ++i)
        {
            auto column = row.removeFromLeft(width).reduced(6);
            labels[i].setBounds(column.removeFromTop(24));
            sliders[i].setBounds(column);
        }
    };
    layoutRow(controls.removeFromTop(compactHeight - 124), 0, bloomStart);
    if (bloom.getToggleState())
        layoutRow(controls.removeFromTop(sectionHeight), bloomStart, swellStart - bloomStart);
    if (swell.getToggleState())
        layoutRow(controls.removeFromTop(sectionHeight), swellStart, sliders.size() - swellStart);
}
}
