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
    shimmerPanel.setClickingTogglesState(true);
    shimmerPanel.onClick = [this] { updateSections(); };
    shimmerVoice.setTitle("Shimmer voice");
    shimmerVoice.onChange = [this] { updateSections(); };
    addAndMakeVisible(shimmerPanel);
    addAndMakeVisible(shimmerVoice);
    const auto configureShimmerSlider = [this](juce::Slider& slider, const juce::String& name)
    {
        slider.setTitle(name);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 22);
        addAndMakeVisible(slider);
    };
    constexpr std::array globalIds { parameters::shimmerMix, parameters::shimmerFeedback };
    constexpr std::array globalNames { "Shimmer Mix", "Feedback" };
    for (std::size_t i = 0; i < globalIds.size(); ++i)
    {
        configureShimmerSlider(shimmerGlobals[i], globalNames[i]);
        shimmerGlobalLabels[i].setText(globalNames[i], juce::dontSendNotification);
        shimmerGlobalLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(shimmerGlobalLabels[i]);
        shimmerAttachments[i] = std::make_unique<SliderAttachment>(owner.state, globalIds[i], shimmerGlobals[i]);
    }
    for (std::size_t voice = 0; voice < Shimmer::voiceCount; ++voice)
    {
        const auto name = "Voice " + juce::String(static_cast<int>(voice + 1));
        shimmerVoice.addItem(name, static_cast<int>(voice + 1));
        for (std::size_t field = 0; field < parameters::voiceParameters.size(); ++field)
        {
            const auto& parameter = parameters::voiceParameters[field];
            const auto i = voice * parameters::voiceParameters.size() + field;
            configureShimmerSlider(voiceSliders[i], name + " " + parameter.label);
            voiceAttachments[i] = std::make_unique<SliderAttachment>(
                owner.state, parameters::voiceId(voice, parameter), voiceSliders[i]);
        }
    }
    for (std::size_t i = 0; i < voiceLabels.size(); ++i)
    {
        voiceLabels[i].setText(parameters::voiceParameters[i].label, juce::dontSendNotification);
        voiceLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(voiceLabels[i]);
    }
    shimmerVoice.setSelectedId(1, juce::dontSendNotification);
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
    const auto shimmerOpen = shimmerPanel.getToggleState();
    shimmerVoice.setVisible(shimmerOpen);
    for (auto& slider : shimmerGlobals)
        slider.setVisible(shimmerOpen);
    for (auto& label : shimmerGlobalLabels)
        label.setVisible(shimmerOpen);
    for (auto& label : voiceLabels)
        label.setVisible(shimmerOpen);
    for (std::size_t i = 0; i < voiceSliders.size(); ++i)
        voiceSliders[i].setVisible(shimmerOpen
            && static_cast<int>(i / voiceLabels.size()) == shimmerVoice.getSelectedItemIndex());
    setSize(getWidth(), compactHeight + sectionHeight * (static_cast<int>(bloomOpen) + static_cast<int>(swellOpen))
        + (shimmerOpen ? 216 : 0));
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
    auto titleRow = controls.removeFromTop(56);
    shimmerPanel.setBounds(titleRow.removeFromRight(96).withSizeKeepingCentre(96, 28));
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
    if (shimmerPanel.getToggleState())
    {
        auto globals = controls.removeFromTop(100);
        shimmerVoice.setBounds(globals.removeFromLeft(126).withSizeKeepingCentre(106, 28));
        for (std::size_t i = 0; i < shimmerGlobals.size(); ++i)
        {
            auto column = globals.removeFromLeft(110);
            shimmerGlobalLabels[i].setBounds(column.removeFromTop(22));
            shimmerGlobals[i].setBounds(column);
        }
        auto row = controls.removeFromTop(116);
        const auto width = row.getWidth() / static_cast<int>(voiceLabels.size());
        for (std::size_t i = 0; i < voiceLabels.size(); ++i)
        {
            auto column = row.removeFromLeft(width);
            voiceLabels[i].setBounds(column.removeFromTop(24));
            for (std::size_t voice = 0; voice < Shimmer::voiceCount; ++voice)
                voiceSliders[voice * voiceLabels.size() + i].setBounds(column);
        }
    }
}
}
