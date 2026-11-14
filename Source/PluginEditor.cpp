#include "PluginEditor.h"

namespace preglow
{
namespace
{
constexpr int voiceRowHeight = 122;
constexpr int voiceHeaderHeight = 48;
constexpr int maxVoiceHeight = 4 * voiceHeaderHeight + voiceRowHeight;
constexpr int controlsWidth = 620;
constexpr int spectrumWidth = 376;
constexpr int columnGap = 12;
constexpr int compactHeight = 250;
constexpr int characterRowHeight = 34;
constexpr int sectionHeight = 136;
constexpr std::size_t bloomStart = 6;
constexpr std::size_t swellStart = 10;
}
PluginEditor::PluginEditor(PluginProcessor& owner) : AudioProcessorEditor(owner), ownerProcessor(owner)
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
    filterPlacement.setTitle("Low Cut Placement");
    filterPlacement.addItemList(parameters::filterPlacements, 1);
    addAndMakeVisible(filterPlacement);
    filterPlacementAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        owner.state, parameters::filterPlacement, filterPlacement);
    lowCut.setTitle("Low Cut");
    lowCut.setSliderStyle(juce::Slider::LinearHorizontal);
    lowCut.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
    lowCutAttachment = std::make_unique<SliderAttachment>(owner.state, parameters::lowCut, lowCut);
    lowCut.setTextValueSuffix(" Hz");
    addAndMakeVisible(lowCut);
    filterPlacement.onChange = [this] { lowCut.setEnabled(filterPlacement.getSelectedId() > 1); };
    filterPlacement.onChange();

    character.setTitle("Reverb character");
    character.addItemList(parameters::characterNames, 1);
    addAndMakeVisible(character);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        owner.state, parameters::character, character);
    character.setTitle("Reverb character A");
    characterB.setTitle("Reverb character B");
    characterB.addItemList(parameters::characterNames, 1);
    addAndMakeVisible(characterB);
    characterBAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        owner.state, parameters::characterB, characterB);
    characterBlend.setTitle("Reverb A/B Blend");
    characterBlend.setSliderStyle(juce::Slider::LinearHorizontal);
    characterBlend.setTextBoxStyle(juce::Slider::TextBoxRight, true, 124, 22);
    addAndMakeVisible(characterBlend);
    characterBlendAttachment = std::make_unique<SliderAttachment>(owner.state, parameters::characterBlend, characterBlend);
    characterBlend.textFromValueFunction = [](double value)
    {
        const auto b = juce::roundToInt(value);
        return "A " + juce::String(100 - b) + "% / B " + juce::String(b) + "%";
    };
    characterBlend.updateText();
    characterALabel.setText("A", juce::dontSendNotification);
    characterBLabel.setText("B", juce::dontSendNotification);
    for (auto* label : { &characterALabel, &characterBLabel })
        addAndMakeVisible(*label);
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
    setSize(controlsWidth + columnGap + spectrumWidth, compactHeight + characterRowHeight);
    startTimerHz(30);
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
    setSize(getWidth(), compactHeight + characterRowHeight + sectionHeight * (static_cast<int>(bloomOpen) + static_cast<int>(swellOpen))
        + (shimmerOpen ? 100 + std::min(maxVoiceHeight, static_cast<int>(voiceDetails.size()) * voiceHeaderHeight + expandedCount * voiceRowHeight) : 0));
    resized();
}

void PluginEditor::timerCallback()
{
    constexpr auto size = PluginProcessor::spectrumSize;
    const auto hasFrame = ownerProcessor.readSpectrum(spectrumFrame, spectrumSampleRate);
    for (std::size_t source = 0; source < spectrum.size(); ++source)
    {
        std::array<float, size / 2 + 1> magnitude {};
        if (hasFrame)
        {
            for (std::size_t channel = 0; channel < 2; ++channel)
            {
                fftWork.fill(0.0f);
                const auto& samples = spectrumFrame[2 * source + channel];
                std::copy(samples.begin(), samples.end(), fftWork.begin());
                spectrumWindow.multiplyWithWindowingTable(fftWork.data(), size);
                spectrumFFT.performFrequencyOnlyForwardTransform(fftWork.data(), true);
                for (std::size_t bin = 0; bin < magnitude.size(); ++bin)
                    magnitude[bin] = std::max(magnitude[bin], fftWork[bin] * (2.0f / size));
            }
        }
        for (std::size_t bin = 0; bin < magnitude.size(); ++bin)
            spectrum[source][bin] = std::max(magnitude[bin], spectrum[source][bin] * 0.89f);
    }
    repaint(spectrumBounds);
}

void PluginEditor::paintSpectrum(juce::Graphics& g)
{
    const auto bounds = spectrumBounds.toFloat();
    const auto plot = bounds.withTrimmedLeft(39).withTrimmedRight(17)
                            .withTrimmedTop(29).withTrimmedBottom(21);
    constexpr float minHz = 20.0f, maxHz = 20000.0f, minDB = -90.0f, maxDB = 6.0f;
    const auto xFor = [&](float hz) { return plot.getX() + plot.getWidth() * std::log(hz / minHz) / std::log(maxHz / minHz); };
    const auto yFor = [&](float db) { return plot.getBottom() - plot.getHeight() * (db - minDB) / (maxDB - minDB); };
    const std::array colours { juce::Colour(0xff60d8ee), juce::Colour(0xffc29bff) };

    g.setColour(juce::Colour(0xff10111e));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setFont(juce::FontOptions(11.0f));
    for (const auto db : { -90, -60, -30, 0 })
    {
        const auto y = yFor(static_cast<float>(db));
        g.setColour(juce::Colour(0xff303247));
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
        g.setColour(juce::Colour(0xffb7b5cd));
        g.drawText(juce::String(db), juce::Rectangle<float>(bounds.getX(), y - 7, 33, 14), juce::Justification::centredRight);
    }
    for (const auto hz : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
    {
        const auto x = xFor(static_cast<float>(hz));
        g.setColour(juce::Colour(0xff303247));
        g.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
        g.setColour(juce::Colour(0xffb7b5cd));
        const auto label = hz >= 1000 ? juce::String(hz / 1000) + "k" : juce::String(hz);
        g.drawText(label, juce::Rectangle<float>(std::min(x - 15, bounds.getRight() - 30),
                                               plot.getBottom() + 3, 30, 15), juce::Justification::centred);
    }
    g.drawText("dBFS", juce::Rectangle<float>(bounds.getX() + 8, bounds.getY() + 6, 35, 15), juce::Justification::centredLeft);
    for (std::size_t source = 0; source < spectrum.size(); ++source)
    {
        g.setColour(colours[source]);
        g.drawText(source == 0 ? "Dry" : "Reverb", juce::Rectangle<float>(bounds.getX() + 47 + 60 * static_cast<float>(source),
                                               bounds.getY() + 6, 59, 15), juce::Justification::centredLeft);
        if (spectrumSampleRate <= 0.0)
            continue;

        juce::Path path;
        const auto width = juce::roundToInt(plot.getWidth());
        for (int pixel = 0; pixel <= width; ++pixel)
        {
            const auto fraction = static_cast<float>(pixel) / static_cast<float>(width);
            const auto hz = minHz * std::pow(maxHz / minHz, fraction);
            const auto bin = static_cast<float>(hz * PluginProcessor::spectrumSize / spectrumSampleRate);
            const auto index = static_cast<std::size_t>(bin);
            if (index + 1 >= spectrum[source].size())
                break;
            const auto magnitude = juce::jmap(bin - static_cast<float>(index),
                                              spectrum[source][index], spectrum[source][index + 1]);
            const auto db = juce::Decibels::gainToDecibels(magnitude, minDB);
            const auto x = plot.getX() + fraction * plot.getWidth();
            const auto y = yFor(juce::jlimit(minDB, maxDB, db));
            if (pixel == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        g.strokePath(path, juce::PathStrokeType(1.4f));
    }
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(0xff151627));
    paintSpectrum(graphics);
    graphics.setColour(juce::Colour(0xfff0eaff));
    graphics.setFont(juce::FontOptions(30.0f));
    graphics.drawText(JucePlugin_Name, getLocalBounds().removeFromTop(68).reduced(24, 0), juce::Justification::centredLeft);
}

void PluginEditor::resized()
{
    auto controls = getLocalBounds().reduced(16);
    spectrumBounds = controls.removeFromRight(spectrumWidth).withHeight(compactHeight - 32);
    controls.removeFromRight(columnGap);
    auto titleRow = controls.removeFromTop(56);
    shimmerPanel.setBounds(titleRow.removeFromRight(96).withSizeKeepingCentre(96, 28));
    titleRow.removeFromRight(8);
    shimmerPower.setBounds(titleRow.removeFromRight(110).withSizeKeepingCentre(110, 28));
    auto characters = controls.removeFromTop(characterRowHeight).reduced(0, 3);
    characterALabel.setBounds(characters.removeFromLeft(22));
    character.setBounds(characters.removeFromLeft(112));
    characters.removeFromLeft(8);
    characterBLabel.setBounds(characters.removeFromLeft(22));
    characterB.setBounds(characters.removeFromLeft(112));
    characters.removeFromLeft(12);
    characterBlend.setBounds(characters);
    auto options = controls.removeFromTop(28);
    freeze.setBounds(options.removeFromLeft(78));
    infinite.setBounds(options.removeFromLeft(123));
    bloom.setBounds(options.removeFromLeft(74));
    swell.setBounds(options.removeFromLeft(72));
    filterPlacement.setBounds(options.removeFromLeft(115));
    lowCut.setBounds(options);
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
