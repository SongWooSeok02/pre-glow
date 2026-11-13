#include "PluginEditor.h"

namespace preglow
{
namespace
{
constexpr int voiceRowHeight = 122;
constexpr int voiceHeaderHeight = 48;
constexpr int maxVoiceHeight = 4 * voiceHeaderHeight + voiceRowHeight;
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
    addAndMakeVisible(shimmerPanel);
    addAndMakeVisible(shimmerPower);
    shimmerPowerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        owner.state, parameters::shimmerEnabled, shimmerPower);
    shimmerRouting.setTitle("Shimmer Routing");
    shimmerRouting.addItemList(parameters::shimmerRoutes, 1);
    addAndMakeVisible(shimmerRouting);
    shimmerRoutingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        owner.state, parameters::shimmerRouting, shimmerRouting);
    const auto updateFeedbackControl = [this]
    {
        shimmerGlobals[1].setEnabled(shimmerRouting.getSelectedId() == 1);
    };
    shimmerRouting.onChange = updateFeedbackControl;
    updateFeedbackControl();
    voiceViewport.setViewedComponent(&voiceContent, false);
    voiceViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(voiceViewport);
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
        auto& toggle = voiceToggles[voice];
        auto& card = voiceCards[voice];
        card.setText(name);
        card.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff575876));
        voiceContent.addAndMakeVisible(card);
        toggle.setButtonText("On");
        toggle.setTitle(name + " on");
        voiceContent.addAndMakeVisible(toggle);
        voiceToggleAttachments[voice] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            owner.state, parameters::voiceId(voice, parameters::voiceEnabled), toggle);

        auto& details = voiceDetails[voice];
        details.setClickingTogglesState(true);
        details.setTitle(name + " controls");
        details.setToggleState(toggle.getToggleState(), juce::dontSendNotification);
        details.onClick = [this] { updateSections(); };
        voiceContent.addAndMakeVisible(details);
        toggle.onStateChange = [this, voice, enabled = toggle.getToggleState()] () mutable
        {
            const auto current = voiceToggles[voice].getToggleState();
            if (current != enabled)
            {
                enabled = current;
                voiceDetails[voice].setToggleState(current, juce::dontSendNotification);
                updateSections();
            }
        };
        for (std::size_t field = 0; field < parameters::voiceParameters.size(); ++field)
        {
            const auto& parameter = parameters::voiceParameters[field];
            const auto i = voice * parameters::voiceParameters.size() + field;
            configureShimmerSlider(voiceSliders[i], name + " " + parameter.label);
            voiceContent.addAndMakeVisible(voiceSliders[i]);
            voiceAttachments[i] = std::make_unique<SliderAttachment>(
                owner.state, parameters::voiceId(voice, parameter), voiceSliders[i]);
        }
    }
    for (std::size_t i = 0; i < voiceLabels.size(); ++i)
    {
        voiceLabels[i].setText(parameters::voiceParameters[i % parameters::voiceParameters.size()].label, juce::dontSendNotification);
        voiceLabels[i].setJustificationType(juce::Justification::centred);
        voiceContent.addAndMakeVisible(voiceLabels[i]);
    }
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
    int expandedCount = 0;
    for (auto& details : voiceDetails)
    {
        const auto expanded = details.getToggleState();
        details.setButtonText(expanded ? "Hide controls" : "Show controls");
        expandedCount += static_cast<int>(expanded);
    }
    shimmerRouting.setVisible(shimmerOpen);
    voiceViewport.setVisible(shimmerOpen);
    for (auto& slider : shimmerGlobals)
        slider.setVisible(shimmerOpen);
    for (auto& label : shimmerGlobalLabels)
        label.setVisible(shimmerOpen);
    for (std::size_t i = 0; i < voiceSliders.size(); ++i)
    {
        const auto visible = voiceDetails[i / parameters::voiceParameters.size()].getToggleState();
        voiceSliders[i].setVisible(visible);
        voiceLabels[i].setVisible(visible);
    }
    setSize(getWidth(), compactHeight + sectionHeight * (static_cast<int>(bloomOpen) + static_cast<int>(swellOpen))
        + (shimmerOpen ? 100 + std::min(maxVoiceHeight, static_cast<int>(voiceDetails.size()) * voiceHeaderHeight + expandedCount * voiceRowHeight) : 0));
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
    titleRow.removeFromRight(8);
    shimmerPower.setBounds(titleRow.removeFromRight(110).withSizeKeepingCentre(110, 28));
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
        shimmerRouting.setBounds(globals.removeFromRight(152).withSizeKeepingCentre(142, 28));

        for (std::size_t i = 0; i < shimmerGlobals.size(); ++i)
        {
            auto column = globals.removeFromLeft(110);
            shimmerGlobalLabels[i].setBounds(column.removeFromTop(22));
            shimmerGlobals[i].setBounds(column);
        }
        const auto expandedCount = std::count_if(voiceDetails.begin(), voiceDetails.end(),
            [](const auto& details) { return details.getToggleState(); });
        const auto height = static_cast<int>(voiceDetails.size()) * voiceHeaderHeight
            + static_cast<int>(expandedCount) * voiceRowHeight;
        voiceViewport.setBounds(controls.removeFromTop(std::min(maxVoiceHeight, height)));
        voiceContent.setSize(voiceViewport.getWidth() - voiceViewport.getScrollBarThickness(), height);
        auto area = voiceContent.getLocalBounds();
        constexpr auto fields = parameters::voiceParameters.size();
        for (std::size_t voice = 0; voice < voiceToggles.size(); ++voice)
        {
            const auto expanded = voiceDetails[voice].getToggleState();
            auto card = area.removeFromTop(voiceHeaderHeight + (expanded ? voiceRowHeight : 0)).reduced(4, 2);
            voiceCards[voice].setBounds(card);
            auto inner = card.reduced(12, 0);
            inner.removeFromBottom(12);
            inner.removeFromTop(14);
            auto header = inner.removeFromTop(25);
            voiceToggles[voice].setBounds(header.removeFromLeft(64));
            voiceDetails[voice].setBounds(header.removeFromRight(106));
            if (!expanded)
                continue;
            auto row = inner;
            const auto width = row.getWidth() / static_cast<int>(fields);
            for (std::size_t field = 0; field < fields; ++field)
            {
                const auto i = voice * fields + field;
                auto column = row.removeFromLeft(width).reduced(4, 0);
                voiceLabels[i].setBounds(column.removeFromTop(24));
                if (voiceSliders[i].getTextBoxWidth() != std::min(64, column.getWidth()))
                    voiceSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, std::min(64, column.getWidth()), 22);
                voiceSliders[i].setBounds(column);
            }
        }
    }
}
}
