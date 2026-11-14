#include "PluginEditor.h"

namespace preglow
{
namespace
{
namespace PreGlowColors
{
const juce::Colour canvas { 0xfff5f7fa };
const juce::Colour surface { 0xffffffff };
const juce::Colour border { 0xffe2e8f0 };
const juce::Colour screen { 0xffeef2f6 };
const juce::Colour goldTint { 0xfffff4d7 };
const juce::Colour goldActive { 0xffe3bc62 };
const juce::Colour goldInk { 0xff76521b };
const juce::Colour coral { 0xffff5e7e };
const juce::Colour cyan { 0xff00e5ff };
const juce::Colour cyanInk { 0xff006b80 };
const juce::Colour text { 0xff1e293b };
const juce::Colour muted { 0xff64748b };
const juce::Colour peach { 0xffff8c42 };
const juce::Colour teal { 0xff147d67 };
const juce::Colour grid { 0xffc4cfdb };
}

struct FactoryPreset
{
    const char* name;
    int character;
    float decay;
    float wet;
};

constexpr FactoryPreset factoryPresets[] {
    { "Hall", 0, 3.0f, 35.0f },
    { "Room", 1, 0.8f, 22.0f },
    { "Plate", 2, 2.4f, 38.0f },
    { "Cloud", 3, 14.0f, 65.0f }
};
constexpr int firstUserPreset = 100;
constexpr int maxPresetBytes = 256 * 1024;
constexpr auto presetExtension = ".preglowpreset";

juce::File presetDirectory()
{
    auto directory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    directory = directory.getChildFile("Application Support");
   #endif
    return directory.getChildFile("pre-glow").getChildFile("Presets");
}

constexpr int voiceRowHeight = 122;
constexpr int voiceHeaderHeight = 48;
constexpr int maxVoiceHeight = 4 * voiceHeaderHeight + voiceRowHeight;
constexpr int controlsWidth = 620;
constexpr int spectrumWidth = 376;
constexpr int columnGap = 12;
constexpr int duckPanelHeight = 116;
constexpr int compactHeight = 250;
constexpr int titleHeight = 44;
constexpr int headerHeight = 88;
constexpr int basicSettingsHeight = 104;
constexpr int basicKnobsHeight = 156;
constexpr int cardGap = 12;
constexpr int editorInset = 16;
constexpr int basicHeight = 2 * editorInset + headerHeight + basicSettingsHeight + cardGap + basicKnobsHeight;
constexpr int morphSize = 220;
constexpr int morphReadoutHeight = 28;
constexpr int morphColumnHeight = 32 + morphSize + morphReadoutHeight;
constexpr int morphButtonRowHeight = 34;
constexpr int meterHeight = 118;
constexpr int meterColumnHeight = 2 * editorInset + compactHeight - 32 + morphButtonRowHeight + meterHeight;
constexpr int stackedMorphHeight = morphColumnHeight + compactHeight - 32 + morphButtonRowHeight + meterHeight;
constexpr std::size_t drySliderIndex = 2;
constexpr std::size_t wetSliderIndex = 3;
constexpr int characterRowHeight = 34;
constexpr int advancedCardPadding = 10;
constexpr int advancedCardRowGap = 6;
constexpr int advancedCardExtraHeight = 2 * advancedCardPadding + advancedCardRowGap;
constexpr int bandRowHeight = 340;
constexpr int shimmerBottomPadding = 12;
constexpr int sectionHeight = 148;
constexpr std::size_t bloomStart = 6;
constexpr std::size_t swellStart = 10;
}
PluginEditor::MorphPad::MorphPad(PluginProcessor& owner)
    : xAttachment(*owner.state.getParameter(parameters::morphX), [this](float x) { position.x = x; repaint(); }),
      yAttachment(*owner.state.getParameter(parameters::morphY), [this](float y) { position.y = y; repaint(); })
{
    setTitle("XY Morph: A/B and Wet. Double-click to centre.");
    xAttachment.sendInitialUpdate();
    yAttachment.sendInitialUpdate();
}

juce::Rectangle<float> PluginEditor::MorphPad::plotBounds() const
{
    return getLocalBounds().toFloat().reduced(26.0f);
}

void PluginEditor::MorphPad::paint(juce::Graphics& g)
{
    const auto plot = plotBounds();
    g.setColour(PreGlowColors::surface);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(PreGlowColors::grid);
    g.drawRect(plot, 1.0f);
    g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());
    g.drawVerticalLine(juce::roundToInt(plot.getCentreX()), plot.getY(), plot.getBottom());
    g.setColour(PreGlowColors::cyan);
    const auto x = plot.getX() + (position.x + 1.0f) * 0.5f * plot.getWidth();
    const auto y = plot.getY() + (1.0f - position.y) * 0.5f * plot.getHeight();
    g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);
    g.setColour(PreGlowColors::text);
    g.setFont(juce::FontOptions("Arial", 14.0f, juce::Font::plain));
    g.drawText("Wet +", getLocalBounds().removeFromTop(22), juce::Justification::centred);
    g.drawText("Wet -", getLocalBounds().removeFromBottom(22), juce::Justification::centred);
    g.drawText("A", getLocalBounds().removeFromLeft(24), juce::Justification::centred);
    g.drawText("B", getLocalBounds().removeFromRight(24), juce::Justification::centred);
}

void PluginEditor::MorphPad::mouseDown(const juce::MouseEvent& event)
{
    xAttachment.beginGesture();
    yAttachment.beginGesture();
    mouseDrag(event);
}

void PluginEditor::MorphPad::mouseDrag(const juce::MouseEvent& event)
{
    const auto plot = plotBounds();
    xAttachment.setValueAsPartOfGesture(juce::jlimit(-1.0f, 1.0f, 2.0f * (event.position.x - plot.getX()) / plot.getWidth() - 1.0f));
    yAttachment.setValueAsPartOfGesture(juce::jlimit(-1.0f, 1.0f, 1.0f - 2.0f * (event.position.y - plot.getY()) / plot.getHeight()));
}

void PluginEditor::MorphPad::mouseUp(const juce::MouseEvent&)
{
    xAttachment.endGesture();
    yAttachment.endGesture();
}

void PluginEditor::MorphPad::mouseDoubleClick(const juce::MouseEvent&)
{
    xAttachment.setValueAsPartOfGesture(0.0f);
    yAttachment.setValueAsPartOfGesture(0.0f);
}

PluginEditor::PluginEditor(PluginProcessor& owner) : AudioProcessorEditor(owner), morphPad(owner), ownerProcessor(owner)
{
    using namespace PreGlowColors;
    lookAndFeel.setColourScheme({ canvas, surface, surface, grid, text, cyan, text, cyan, text });
    lookAndFeel.setColour(juce::Slider::backgroundColourId, border);
    lookAndFeel.setColour(juce::Slider::trackColourId, cyan);
    lookAndFeel.setColour(juce::Slider::thumbColourId, cyanInk);
    lookAndFeel.setDefaultSansSerifTypefaceName("Arial");
    for (auto* button : { &advancedMode, &duckPanel, &bandPanel, &shimmerPanel })
    {
        button->setColour(juce::TextButton::buttonColourId, goldTint);
        button->setColour(juce::TextButton::buttonOnColourId, goldActive);
        button->setColour(juce::TextButton::textColourOffId, goldInk);
    }
    shimmerPower.setColour(juce::ToggleButton::tickColourId, goldInk);
    freeze.setColour(juce::ToggleButton::tickColourId, peach);

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
        slider.setColour(juce::Slider::rotarySliderFillColourId, i == 0 ? PreGlowColors::coral : PreGlowColors::cyan);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, PreGlowColors::border);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setTitle(names[i]);
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(labels[i]);
        addAndMakeVisible(slider);
        if (i != wetSliderIndex)
            attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(owner.state, ids[i], slider);
    }
    bandPanel.setClickingTogglesState(true);
    bandPanel.onClick = [this] { updateSections(); };
    addAndMakeVisible(bandPanel);
    for (std::size_t band = 0; band < bandDecay.size(); ++band)
    {
        auto& slider = bandDecay[band];
        slider.setTitle(parameters::bandDecayNames[band]);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
        bandLabels[band].setText(parameters::bandDecayNames[band], juce::dontSendNotification);
        bandAttachments[band] = std::make_unique<SliderAttachment>(owner.state, parameters::bandDecayIds[band], slider);
        slider.setTextValueSuffix("x");
        addAndMakeVisible(slider);
        addAndMakeVisible(bandLabels[band]);
        auto& dampingSlider = bandDamping[band];
        dampingSlider.setTitle(parameters::bandDampingNames[band]);
        dampingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
        bandDampingLabels[band].setText(parameters::bandDampingNames[band], juce::dontSendNotification);
        bandDampingAttachments[band] = std::make_unique<SliderAttachment>(owner.state, parameters::bandDampingIds[band], dampingSlider);
        dampingSlider.setTextValueSuffix("%");
        addAndMakeVisible(dampingSlider);
        addAndMakeVisible(bandDampingLabels[band]);
        auto& levelSlider = bandLevel[band];
        levelSlider.setTitle(parameters::bandLevelNames[band]);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
        bandLevelLabels[band].setText(parameters::bandLevelNames[band], juce::dontSendNotification);
        bandLevelAttachments[band] = std::make_unique<SliderAttachment>(owner.state, parameters::bandLevelIds[band], levelSlider);
        levelSlider.setTextValueSuffix("%");
        addAndMakeVisible(levelSlider);
        addAndMakeVisible(bandLevelLabels[band]);
        auto& sendSlider = bandSend[band];
        sendSlider.setTitle(parameters::bandSendNames[band]);
        sendSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
        bandSendLabels[band].setText(parameters::bandSendNames[band], juce::dontSendNotification);
        bandSendAttachments[band] = std::make_unique<SliderAttachment>(owner.state, parameters::bandSendIds[band], sendSlider);
        sendSlider.setTextValueSuffix("%");
        addAndMakeVisible(sendSlider);
        addAndMakeVisible(bandSendLabels[band]);
        auto& modSlider = bandMod[band];
        modSlider.setTitle(parameters::bandModNames[band]);
        modSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 22);
        bandModLabels[band].setText(parameters::bandModNames[band], juce::dontSendNotification);
        bandModAttachments[band] = std::make_unique<SliderAttachment>(owner.state, parameters::bandModIds[band], modSlider);
        modSlider.setTextValueSuffix("%");
        addAndMakeVisible(modSlider);
        addAndMakeVisible(bandModLabels[band]);
    }
    for (std::size_t i = 0; i < crossovers.size(); ++i)
    {
        auto& slider = crossovers[i];
        slider.setTitle(parameters::crossoverNames[i]);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 74, 22);
        crossoverLabels[i].setText(parameters::crossoverNames[i], juce::dontSendNotification);
        crossoverAttachments[i] = std::make_unique<SliderAttachment>(owner.state, parameters::crossoverIds[i], slider);
        slider.setTextValueSuffix(" Hz");
        addAndMakeVisible(slider);
        addAndMakeVisible(crossoverLabels[i]);
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
    bindMorphSlider(characterBlend, parameters::morphX, characterBlendAttachment);
    bindMorphSlider(sliders[wetSliderIndex], parameters::morphY, wetMorphAttachment);
    sliders[wetSliderIndex].setTextValueSuffix("%");
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
    addChildComponent(shimmerSignalPath);
    shimmerSignalPath.setTitle("Wet signal path");
    shimmerSignalPath.setJustificationType(juce::Justification::centred);
    const auto updateRouting = [this]
    {
        const auto route = shimmerRouting.getSelectedItemIndex();
        updateShimmerFeedbackVisibility();
        const std::array<juce::String, 4> paths {
            juce::String::fromUTF8("Input → Reverb → Wet   •   Reverb → Shimmer → Reverb"),
            juce::String::fromUTF8("Input → Reverb → Wet   •   Reverb → Shimmer → Wet"),
            juce::String::fromUTF8("Input → Reverb → Wet   •   Input → Shimmer → Reverb"),
            juce::String::fromUTF8("Input → Reverb → Wet   •   Input → Shimmer → Wet")
        };
        shimmerSignalPath.setText(shimmerPower.getToggleState() && juce::isPositiveAndBelow(route, 4)
                                      ? paths[static_cast<std::size_t>(route)] : juce::String::fromUTF8("Input → Reverb → Wet"),
                                  juce::dontSendNotification);
    };
    shimmerRouting.onChange = shimmerPower.onStateChange = updateRouting;
    updateRouting();
    voiceViewport.setViewedComponent(&voiceContent, false);
    voiceViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(voiceViewport);
    const auto configureShimmerSlider = [this](juce::Slider& slider, const juce::String& name)
    {
        slider.setTitle(name);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 22);
        slider.setColour(juce::Slider::rotarySliderFillColourId, PreGlowColors::cyanInk);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, PreGlowColors::grid);
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
        card.setColour(juce::GroupComponent::outlineColourId, PreGlowColors::border);
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
    duckOnBinding = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(owner.state, parameters::duckEnabled, duckOn);
    reverseOnBinding = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(owner.state, parameters::reverseEnabled, reverseOn);
    reverseTime.setTitle("Reverse Rise");
    reverseTime.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    reverseTimeBinding = std::make_unique<SliderAttachment>(owner.state, parameters::reverseRise, reverseTime);
    reverseTime.setTextValueSuffix(" ms");
    reverseLabel.setText("Rise", juce::dontSendNotification);
    addChildComponent(duckOn);
    addAndMakeVisible(reverseOn);
    addAndMakeVisible(reverseTime);
    addAndMakeVisible(reverseLabel);
    duckPanel.setClickingTogglesState(true);
    duckPanel.onClick = [this] { updateSections(); };
    addAndMakeVisible(duckPanel);
    constexpr std::array suffixes { " dB", "%", " ms", " ms" };
    for (std::size_t i = 0; i < duckSliders.size(); ++i)
    {
        auto& slider = duckSliders[i];
        slider.setTitle(juce::String("Ducking ") + parameters::duckNames[i]);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 22);
        duckLabels[i].setText(parameters::duckNames[i], juce::dontSendNotification);
        duckAttachments[i] = std::make_unique<SliderAttachment>(owner.state, parameters::duckIds[i], slider);
        slider.setTextValueSuffix(suffixes[i]);
        addChildComponent(slider);
        addChildComponent(duckLabels[i]);
    }
    advancedMode.setClickingTogglesState(true);
    advancedMode.onClick = [this] { updateSections(); };
    addAndMakeVisible(advancedMode);
    morphPanel.setClickingTogglesState(true);
    morphPanel.onClick = [this] { updateSections(); };
    addAndMakeVisible(morphPanel);
    addChildComponent(morphPad);
    morphReadout.setJustificationType(juce::Justification::centred);
    addChildComponent(morphReadout);
    configurePresets();
    setLookAndFeel(&lookAndFeel);
    updateMorphControls();
    startTimerHz(30);
    updateSections();
}

void PluginEditor::configurePresets()
{
    addAndMakeVisible(presetButton);
    addChildComponent(presetPanel);
    for (auto* component : std::initializer_list<juce::Component*> {
             &presetCategory, &presetSearch, &presetChoices, &presetLoad, &presetSave,
             &presetRename, &presetDelete, &presetStatus })
        presetPanel.addAndMakeVisible(*component);

    presetCategory.setTitle("Preset category");
    presetCategory.addItemList({ "All Presets", "Factory", "User" }, 1);
    presetCategory.setSelectedId(1, juce::dontSendNotification);
    presetCategory.onChange = [this] { refreshPresets(); };
    presetSearch.setTitle("Search presets");
    presetSearch.setTextToShowWhenEmpty("Search presets", PreGlowColors::muted);
    presetSearch.onTextChange = [this] { refreshPresets(); };
    presetChoices.setTitle("Preset");
    presetChoices.setTextWhenNothingSelected("Select a preset");
    presetChoices.onChange = [this]
    {
        const auto user = selectedUserPreset() != juce::File {};
        presetLoad.setEnabled(presetChoices.getSelectedId() != 0);
        presetRename.setEnabled(user);
        presetDelete.setEnabled(user);
    };
    presetLoad.onClick = [this] { loadSelectedPreset(); };
    presetSave.onClick = [this] { editPresetName(false); };
    presetRename.onClick = [this] { editPresetName(true); };
    presetDelete.onClick = [this] { deleteSelectedPreset(); };
    presetButton.onClick = [this]
    {
        const auto opening = !presetPanel.isVisible();
        if (opening)
            refreshPresets();
        presetPanel.setVisible(opening);
        presetButton.setToggleState(opening, juce::dontSendNotification);
        repaint(spectrumBounds);
    };
}

void PluginEditor::refreshPresets()
{
    const auto previous = presetChoices.getText();
    const auto query = presetSearch.getText().trim();
    const auto category = presetCategory.getSelectedId();
    presetChoices.clear(juce::dontSendNotification);
    listedUserPresets.clear();
    if (category != 3)
        for (std::size_t i = 0; i < std::size(factoryPresets); ++i)
        {
            const juce::String label = juce::String(factoryPresets[i].name) + " (Factory)";
            if (query.isEmpty() || label.containsIgnoreCase(query))
                presetChoices.addItem(label, static_cast<int>(i) + 1);
        }
    if (category != 2)
    {
        auto files = presetDirectory().findChildFiles(juce::File::findFiles | juce::File::ignoreHiddenFiles,
                                                      false, "*" + juce::String(presetExtension));
        files.sort();
        for (const auto& file : files)
        {
            const auto label = file.getFileNameWithoutExtension() + " (User)";
            if (query.isNotEmpty() && !label.containsIgnoreCase(query))
                continue;
            presetChoices.addItem(label, firstUserPreset + listedUserPresets.size());
            listedUserPresets.add(file);
        }
    }
    for (int i = 0; i < presetChoices.getNumItems(); ++i)
        if (presetChoices.getItemText(i) == previous)
            presetChoices.setSelectedItemIndex(i, juce::dontSendNotification);
    presetChoices.onChange();
    showPresetStatus(presetChoices.getNumItems() == 0 ? "No matching presets." : "");
}

juce::File PluginEditor::selectedUserPreset() const
{
    const auto index = presetChoices.getSelectedId() - firstUserPreset;
    return juce::isPositiveAndBelow(index, listedUserPresets.size())
        ? listedUserPresets[index] : juce::File {};
}

void PluginEditor::loadSelectedPreset()
{
    const auto id = presetChoices.getSelectedId();
    if (id == 0)
        return;
    if (id < firstUserPreset)
    {
        const auto& preset = factoryPresets[id - 1];
        ownerProcessor.loadFactoryPreset(preset.character, preset.decay, preset.wet);
    }
    else
    {
        const auto file = selectedUserPreset();
        juce::FileInputStream stream(file);
        const auto size = stream.getTotalLength();
        juce::MemoryBlock data;
        if (!stream.openedOk() || size <= 0 || size > maxPresetBytes
            || stream.readIntoMemoryBlock(data, static_cast<ssize_t>(size)) != static_cast<std::size_t>(size)
            || !ownerProcessor.loadPreset(data))
        {
            showPresetStatus("Could not load this preset.");
            return;
        }
    }
    updateSections();
    showPresetStatus("Loaded " + presetChoices.getText());
}

void PluginEditor::editPresetName(bool rename)
{
    const auto original = selectedUserPreset();
    if (rename && original == juce::File {})
        return;
    auto dialog = std::make_unique<juce::AlertWindow>(rename ? "Rename Preset" : "Save Preset",
                                                     "Enter a user preset name.", juce::MessageBoxIconType::NoIcon, this);
    dialog->addTextEditor("name", rename ? original.getFileNameWithoutExtension() : juce::String {}, "Name");
    dialog->addButton(rename ? "Rename" : "Save", 1, juce::KeyPress { juce::KeyPress::returnKey });
    dialog->addButton("Cancel", 0, juce::KeyPress { juce::KeyPress::escapeKey });
    auto* window = dialog.release();
    window->enterModalState(true, juce::ModalCallbackFunction::create(
        [editor = juce::Component::SafePointer<PluginEditor>(this),
         alert = juce::Component::SafePointer<juce::AlertWindow>(window), original, rename](int result)
        {
            if (result != 1 || editor == nullptr || alert == nullptr)
                return;
            const auto name = alert->getTextEditorContents("name").trim();
            if (name.isEmpty() || name.startsWithChar('.') || name.endsWithChar('.')
                || name.getNumBytesAsUTF8() > 120 || juce::File::createLegalFileName(name) != name
                || name.containsAnyOf("\t\r\n"))
            {
                editor->showPresetStatus("Choose a valid preset name.");
                return;
            }

            const auto directory = presetDirectory();
            const auto target = directory.getChildFile(name + presetExtension);
            if (rename && target == original && name == original.getFileNameWithoutExtension())
                return;
            if (target.exists())
            {
                editor->showPresetStatus("A preset with that name already exists.");
                return;
            }
            if (rename)
            {
                if (!original.existsAsFile() || !original.moveFileTo(target))
                {
                    editor->showPresetStatus("Could not rename this preset.");
                    return;
                }
            }
            else
            {
                juce::MemoryBlock data;
                editor->ownerProcessor.getStateInformation(data);
                if (directory.createDirectory().failed() || data.isEmpty()
                    || !target.replaceWithData(data.getData(), data.getSize()))
                {
                    editor->showPresetStatus("Could not save this preset.");
                    return;
                }
            }
            editor->presetCategory.setSelectedId(3, juce::dontSendNotification);
            editor->presetSearch.clear();
            editor->refreshPresets();
            const auto index = editor->listedUserPresets.indexOf(target);
            if (index >= 0)
                editor->presetChoices.setSelectedId(firstUserPreset + index, juce::sendNotificationSync);
            editor->showPresetStatus((rename ? "Renamed " : "Saved ") + name);
        }), true);
}

void PluginEditor::deleteSelectedPreset()
{
    const auto file = selectedUserPreset();
    if (file == juce::File {})
        return;
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, "Delete Preset",
        "Delete " + file.getFileNameWithoutExtension() + "?", "Delete", "Cancel", this,
        juce::ModalCallbackFunction::create([editor = juce::Component::SafePointer<PluginEditor>(this), file](int result)
        {
            if (result != 1 || editor == nullptr)
                return;
            const auto deleted = file.existsAsFile() && file.moveToTrash();
            editor->refreshPresets();
            editor->showPresetStatus(deleted ? "Preset moved to Trash." : "Could not delete this preset.");
        }));
}

void PluginEditor::showPresetStatus(const juce::String& message)
{
    presetStatus.setText(message, juce::dontSendNotification);
}

void PluginEditor::updateSections()
{
    const auto advanced = advancedMode.getToggleState();
    advancedMode.setButtonText(advanced ? "Advanced" : "Basic");
    advancedMode.setTitle(advanced ? "Switch to Basic mode" : "Switch to Advanced mode");
    for (auto* component : std::initializer_list<juce::Component*> {
             &infinite, &bloom, &swell, &filterPlacement, &lowCut, &reverseOn, &reverseTime,
             &reverseLabel, &shimmerPanel, &bandPanel, &duckPanel, &morphPanel })
        component->setVisible(advanced);

    for (std::size_t i = 0; i < bloomStart; ++i)
    {
        const auto visible = advanced || i == 0 || i == drySliderIndex || i == wetSliderIndex;
        labels[i].setVisible(visible);
        sliders[i].setVisible(visible);
    }

    const auto bloomOpen = advanced && bloom.getToggleState();
    const auto swellOpen = advanced && swell.getToggleState();
    for (std::size_t i = bloomStart; i < sliders.size(); ++i)
    {
        const auto visible = i < swellStart ? bloomOpen : swellOpen;
        labels[i].setVisible(visible);
        sliders[i].setVisible(visible);
    }
    const auto shimmerOpen = advanced && shimmerPanel.getToggleState();
    int expandedCount = 0;
    for (auto& details : voiceDetails)
    {
        const auto expanded = details.getToggleState();
        details.setButtonText(expanded ? "Hide controls" : "Show controls");
        expandedCount += static_cast<int>(expanded);
    }
    shimmerRouting.setVisible(shimmerOpen);
    shimmerSignalPath.setVisible(shimmerOpen);
    voiceViewport.setVisible(shimmerOpen);
    shimmerGlobals[0].setVisible(shimmerOpen);
    shimmerGlobalLabels[0].setVisible(shimmerOpen);
    updateShimmerFeedbackVisibility();
    for (std::size_t i = 0; i < voiceSliders.size(); ++i)
    {
        const auto visible = voiceDetails[i / parameters::voiceParameters.size()].getToggleState();
        voiceSliders[i].setVisible(visible);
        voiceLabels[i].setVisible(visible);
    }
    const auto bandsOpen = advanced && bandPanel.getToggleState();
    for (std::size_t band = 0; band < bandDecay.size(); ++band)
    {
        bandDecay[band].setVisible(bandsOpen);
        bandLabels[band].setVisible(bandsOpen);
        bandDamping[band].setVisible(bandsOpen);
        bandDampingLabels[band].setVisible(bandsOpen);
        bandLevel[band].setVisible(bandsOpen);
        bandLevelLabels[band].setVisible(bandsOpen);
        bandSend[band].setVisible(bandsOpen);
        bandSendLabels[band].setVisible(bandsOpen);
        bandMod[band].setVisible(bandsOpen);
        bandModLabels[band].setVisible(bandsOpen);
    }
    for (std::size_t i = 0; i < crossovers.size(); ++i)
    {
        crossovers[i].setVisible(bandsOpen);
        crossoverLabels[i].setVisible(bandsOpen);
    }
    const auto morphOpen = advanced && morphPanel.getToggleState();
    morphPanel.setButtonText(morphOpen ? "Hide XY Morph" : "Show XY Morph");
    morphPad.setVisible(morphOpen);
    morphReadout.setVisible(morphOpen);
    const auto duckOpen = advanced && duckPanel.getToggleState();
    duckOn.setVisible(duckOpen);
    for (std::size_t i = 0; i < duckSliders.size(); ++i)
    {
        duckSliders[i].setVisible(duckOpen);
        duckLabels[i].setVisible(duckOpen);
    }
    const auto morphBelow = bandsOpen || shimmerOpen;
    const auto width = controlsWidth + columnGap + spectrumWidth + (morphOpen && !morphBelow ? columnGap + morphSize : 0);
    if (!advanced)
    {
        setSize(width, basicHeight);
        resized();
        repaint();
        return;
    }
    setSize(width, std::max(morphOpen && morphBelow ? stackedMorphHeight : meterColumnHeight, (duckOpen ? duckPanelHeight : 0) + (bandsOpen ? bandRowHeight : 0) + compactHeight + characterRowHeight + advancedCardExtraHeight + 30 + (headerHeight - 56) + sectionHeight * (static_cast<int>(bloomOpen) + static_cast<int>(swellOpen))
        + (shimmerOpen ? 132 + shimmerBottomPadding + std::min(maxVoiceHeight, static_cast<int>(voiceDetails.size()) * voiceHeaderHeight + expandedCount * voiceRowHeight) : 0)));
    resized();
    repaint();
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
    updateMorphControls();
    repaint(spectrumBounds);

    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto elapsed = now - lastMeterUpdate;
    const auto release = static_cast<float>(std::pow(0.1, elapsed / 600.0));
    lastMeterUpdate = now;
    unsigned revision = 0;
    const auto peaks = ownerProcessor.readMeterPeaks(revision);
    if (revision != lastMeterRevision)
    {
        std::copy_n(peaks.begin(), displayedPeaks.size(), displayedPeaks.begin());
        const auto target = peaks.back();
        if (target <= 1.0f)
        {
            const auto weight = static_cast<float>(-std::expm1(-elapsed / 350.0));
            displayedCorrelation = displayedCorrelation > 1.0f
                ? target : displayedCorrelation + weight * (target - displayedCorrelation);
        }
        if (target <= 1.0f)
            lastMeterFrame = now;
    }
    else
        for (auto& peak : displayedPeaks)
            peak *= release; // The host can stop calling processBlock when transport stops.
    if (now - lastMeterFrame > 250.0)
        displayedCorrelation = 2.0f;
    lastMeterRevision = revision;
    repaint(meterBounds);
}

void PluginEditor::bindMorphSlider(juce::Slider& slider, const char* axisId,
                                  std::unique_ptr<juce::ParameterAttachment>& attachment)
{
    slider.setRange(0.0, 100.0, 1.0);
    attachment = std::make_unique<juce::ParameterAttachment>(*ownerProcessor.state.getParameter(axisId),
        [this](float) { updateMorphControls(); });
    auto* axis = attachment.get();
    slider.onDragStart = [axis] { axis->beginGesture(); };
    slider.onDragEnd = [axis] { axis->endGesture(); };
    slider.onValueChange = [&slider, axis]
    {
        const auto position = static_cast<float>(slider.getValue() * 0.02 - 1.0);
        if (slider.isMouseButtonDown())
            axis->setValueAsPartOfGesture(position);
        else
            axis->setValueAsCompleteGesture(position);
    };
}

void PluginEditor::updateMorphControls()
{
    const auto blend = 100.0f * ownerProcessor.effectiveBlend();
    const auto wet = 100.0f * ownerProcessor.effectiveWet();
    characterBlend.setValue(blend, juce::dontSendNotification);
    sliders[wetSliderIndex].setValue(wet, juce::dontSendNotification);
    morphReadout.setText("XY Morph", juce::dontSendNotification);
}

void PluginEditor::paintSpectrum(juce::Graphics& g)
{
    const auto bounds = spectrumBounds.toFloat();
    const auto plot = bounds.withTrimmedLeft(39).withTrimmedRight(17)
                            .withTrimmedTop(29).withTrimmedBottom(21);
    constexpr float minHz = 20.0f, maxHz = 20000.0f, minDB = -90.0f, maxDB = 6.0f;
    const auto xFor = [&](float hz) { return plot.getX() + plot.getWidth() * std::log(hz / minHz) / std::log(maxHz / minHz); };
    const auto yFor = [&](float db) { return plot.getBottom() - plot.getHeight() * (db - minDB) / (maxDB - minDB); };
    const std::array colours { juce::Colour(0xffffd700), juce::Colour(0xff00ffff) };

    g.setColour(juce::Colour(0xff25313e));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setFont(juce::FontOptions("Arial", 11.0f, juce::Font::plain));
    if (bandPanel.isVisible() && bandPanel.getToggleState())
    {
        const std::array edges { plot.getX(), xFor(static_cast<float>(crossovers[0].getValue())),
                                 xFor(static_cast<float>(crossovers[1].getValue())), plot.getRight() };
        constexpr std::array names { "Low", "Mid", "High" };
        const std::array bandColours { colours[0], colours[1], colours[0] };
        for (std::size_t band = 0; band < names.size(); ++band)
        {
            const auto region = plot.withX(edges[band]).withWidth(edges[band + 1] - edges[band]);
            g.setColour(bandColours[band].withAlpha(0.10f));
            g.fillRect(region);
            g.setColour(bandColours[band]);
            g.drawText(names[band], region.withTrimmedTop(region.getHeight() - 17.0f).reduced(2.0f, 0.0f),
                       juce::Justification::centred);
            if (band > 0)
            {
                g.setColour(bandColours[band].withAlpha(0.65f));
                g.fillRect(region.withWidth(1.0f));
            }
        }
    }
    for (const auto db : { -90, -60, -30, 0 })
    {
        const auto y = yFor(static_cast<float>(db));
        g.setColour(juce::Colour(0xff435364));
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
        g.setColour(PreGlowColors::border);
        g.drawText(juce::String(db), juce::Rectangle<float>(bounds.getX(), y - 7, 33, 14), juce::Justification::centredRight);
    }
    for (const auto hz : { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 })
    {
        const auto x = xFor(static_cast<float>(hz));
        g.setColour(juce::Colour(0xff435364));
        g.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
        g.setColour(PreGlowColors::border);
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

void PluginEditor::paintMeters(juce::Graphics& graphics)
{
    auto rows = meterBounds.reduced(10, 5);
    graphics.setColour(PreGlowColors::surface);
    graphics.fillRoundedRectangle(meterBounds.toFloat(), 6.0f);
    graphics.setFont(juce::FontOptions("Arial", 12.0f, juce::Font::plain));
    constexpr std::array meterLabels { "Input", "Output", "Tail" };
    const std::array colours { PreGlowColors::coral, PreGlowColors::cyan, PreGlowColors::teal };
    for (std::size_t i = 0; i < displayedPeaks.size(); ++i)
    {
        auto row = rows.removeFromTop(24);
        const auto db = juce::Decibels::gainToDecibels(displayedPeaks[i], -60.0f);
        graphics.setColour(PreGlowColors::text);
        graphics.drawText(meterLabels[i], row.removeFromLeft(54), juce::Justification::centredLeft);
        const auto level = db <= -60.0f ? juce::String("< -60") : juce::String(db, 1);
        graphics.drawText(level + " dBFS", row.removeFromRight(84), juce::Justification::centredRight);
        const auto track = row.withSizeKeepingCentre(row.getWidth(), 8).toFloat();
        graphics.setColour(PreGlowColors::border);
        graphics.fillRoundedRectangle(track, 4.0f);
        const auto fraction = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
        graphics.setColour(db >= 0.0f ? PreGlowColors::peach
                                     : colours[i]);
        graphics.fillRoundedRectangle(track.withWidth(track.getWidth() * fraction), 4.0f);
    }
    auto row = rows.removeFromTop(24);
    graphics.setColour(PreGlowColors::text);
    graphics.drawText("Stereo", row.removeFromLeft(54), juce::Justification::centredLeft);
    const auto active = displayedCorrelation <= 1.0f;
    graphics.drawText(active ? juce::String(displayedCorrelation, 1) : juce::String("-"),
                      row.removeFromRight(84), juce::Justification::centredRight);
    const auto track = row.withSizeKeepingCentre(row.getWidth(), 8).toFloat();
    graphics.setColour(PreGlowColors::border);
    graphics.fillRoundedRectangle(track, 4.0f);
    graphics.setColour(PreGlowColors::muted);
    graphics.drawVerticalLine(juce::roundToInt(track.getCentreX()), track.getY(), track.getBottom());
    if (active)
    {
        const auto x = track.getX() + (displayedCorrelation + 1.0f) * 0.5f * track.getWidth();
        graphics.setColour(displayedCorrelation < 0.0f ? PreGlowColors::peach : colours[0]);
        const auto centre = track.getCentreX();
        graphics.fillRect(track.withX(std::min(x, centre)).withWidth(std::abs(x - centre)));
    }
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(PreGlowColors::canvas);
    if (presetPanel.isVisible())
    {
        graphics.setColour(PreGlowColors::surface);
        graphics.fillRoundedRectangle(spectrumBounds.toFloat(), 8.0f);
    }
    else
        paintSpectrum(graphics);
    paintMeters(graphics);
    for (const auto bounds : sectionBounds)
    {
        if (bounds.isEmpty())
            continue;
        const auto box = bounds.toFloat().reduced(0.5f, advancedMode.getToggleState() ? 2.5f : 0.5f);
        graphics.setColour(PreGlowColors::surface);
        graphics.fillRoundedRectangle(box, 8.0f);
        graphics.setColour(PreGlowColors::border);
        graphics.drawRoundedRectangle(box, 8.0f, 1.0f);
    }
    graphics.setColour(PreGlowColors::text);
    graphics.setFont(juce::FontOptions("Arial", 30.0f, juce::Font::bold).withKerningFactor(0.09f));
    graphics.drawText(JucePlugin_Name, getLocalBounds().reduced(editorInset).removeFromTop(titleHeight), juce::Justification::centredLeft);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel(nullptr);
}

void PluginEditor::updateShimmerFeedbackVisibility()
{
    const auto showFeedback = shimmerSignalPath.isVisible() && shimmerRouting.getSelectedItemIndex() == 0;
    shimmerGlobals[1].setVisible(showFeedback);
    shimmerGlobalLabels[1].setVisible(showFeedback);
}

void PluginEditor::resized()
{
    auto controls = getLocalBounds().reduced(editorInset);
    const auto advanced = advancedMode.getToggleState();
    const auto morphOpen = advanced && morphPanel.getToggleState();
    const auto morphBelow = bandPanel.getToggleState() || shimmerPanel.getToggleState();
    juce::Rectangle<int> morphColumn;
    if (morphOpen && !morphBelow)
    {
        morphColumn = controls.removeFromRight(morphSize);
        controls.removeFromRight(columnGap);
    }

    auto spectrumColumn = controls.removeFromRight(spectrumWidth);
    spectrumBounds = spectrumColumn.removeFromTop(compactHeight - 32);
    auto spectrumActions = spectrumColumn.removeFromTop(morphButtonRowHeight);
    meterBounds = spectrumColumn.removeFromTop(meterHeight).withTrimmedBottom(8);
    presetButton.setBounds(spectrumActions.removeFromRight(100).withSizeKeepingCentre(92, 26));
    presetPanel.setBounds(spectrumBounds);
    auto presetArea = presetPanel.getLocalBounds().reduced(14);
    presetCategory.setBounds(presetArea.removeFromTop(28));
    presetArea.removeFromTop(8);
    presetSearch.setBounds(presetArea.removeFromTop(28));
    presetArea.removeFromTop(8);
    presetChoices.setBounds(presetArea.removeFromTop(28));
    presetArea.removeFromTop(8);
    auto presetActions = presetArea.removeFromTop(28);
    for (auto* button : { &presetLoad, &presetSave, &presetRename, &presetDelete })
        button->setBounds(presetActions.removeFromLeft(87).reduced(2, 0));
    presetStatus.setBounds(presetArea.removeFromTop(44));
    morphPanel.setBounds(spectrumActions.removeFromLeft(144).withSizeKeepingCentre(128, 26));
    if (morphOpen)
    {
        if (morphBelow)
            morphColumn = spectrumColumn.withSizeKeepingCentre(morphSize, spectrumColumn.getHeight());
        morphReadout.setBounds(morphColumn.removeFromTop(morphReadoutHeight));
        morphPad.setBounds(morphColumn.removeFromTop(morphSize));
    }
    controls.removeFromRight(columnGap);
    auto editorHeader = controls.removeFromTop(headerHeight);
    editorHeader.removeFromTop(titleHeight);
    auto titleRow = editorHeader.removeFromTop(30);
    advancedMode.setBounds(titleRow.removeFromLeft(110));
    sectionBounds.fill({});
    if (!advanced)
    {
        sectionBounds[0] = controls.removeFromTop(basicSettingsHeight);
        auto settings = sectionBounds[0].reduced(16, 12);
        auto characters = settings.removeFromTop(30);
        characterALabel.setBounds(characters.removeFromLeft(22));
        character.setBounds(characters.removeFromLeft(112));
        characters.removeFromLeft(12);
        characterBLabel.setBounds(characters.removeFromLeft(22));
        characterB.setBounds(characters.removeFromLeft(112));
        characters.removeFromLeft(16);
        characterBlend.setBounds(characters);

        settings.removeFromTop(12);
        auto options = settings.removeFromTop(28);
        freeze.setBounds(options.removeFromLeft(90));
        shimmerPower.setBounds(options.removeFromLeft(120));

        controls.removeFromTop(cardGap);
        sectionBounds[1] = controls.removeFromTop(basicKnobsHeight);
        auto row = sectionBounds[1].reduced(16, 14);
        const auto width = row.getWidth() / 3;
        for (const auto i : { std::size_t { 0 }, drySliderIndex, wetSliderIndex })
        {
            auto column = row.removeFromLeft(width).reduced(12, 0);
            labels[i].setBounds(column.removeFromTop(24));
            sliders[i].setBounds(column.withTrimmedTop(4));
        }
        return;
    }

    shimmerPanel.setBounds(titleRow.removeFromRight(96).withSizeKeepingCentre(96, 28));
    titleRow.removeFromRight(8);
    shimmerPower.setBounds(titleRow.removeFromRight(110).withSizeKeepingCentre(110, 28));
    bandPanel.setBounds(titleRow.removeFromRight(96).withSizeKeepingCentre(88, 28));
    duckPanel.setBounds(titleRow.removeFromRight(90).withSizeKeepingCentre(82, 28));
    sectionBounds[0] = controls.withHeight(characterRowHeight + 28 + advancedCardExtraHeight);
    controls.removeFromTop(advancedCardPadding);
    auto characters = controls.removeFromTop(characterRowHeight).reduced(10, 4);
    characterALabel.setBounds(characters.removeFromLeft(22));
    character.setBounds(characters.removeFromLeft(112));
    characters.removeFromLeft(8);
    characterBLabel.setBounds(characters.removeFromLeft(22));
    characterB.setBounds(characters.removeFromLeft(112));
    characters.removeFromLeft(12);
    characterBlend.setBounds(characters);
    controls.removeFromTop(advancedCardRowGap);
    auto options = controls.removeFromTop(28).reduced(10, 2);
    freeze.setBounds(options.removeFromLeft(78));
    infinite.setBounds(options.removeFromLeft(123));
    bloom.setBounds(options.removeFromLeft(74));
    swell.setBounds(options.removeFromLeft(72));
    filterPlacement.setBounds(options.removeFromLeft(115));
    lowCut.setBounds(options);
    controls.removeFromTop(advancedCardPadding + 8);
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
    auto reverseRow = controls.removeFromTop(30);
    reverseOn.setBounds(reverseRow.removeFromLeft(116));
    reverseLabel.setBounds(reverseRow.removeFromLeft(36));
    reverseTime.setBounds(reverseRow.removeFromLeft(230).reduced(4, 0));
    sectionBounds[1] = controls.removeFromTop(compactHeight - 124);
    layoutRow(sectionBounds[1].reduced(12, 10), 0, bloomStart);
    if (bandPanel.getToggleState())
    {
        sectionBounds[2] = controls.removeFromTop(bandRowHeight);
        auto bandRow = sectionBounds[2].reduced(12, 14);
        auto frequencyRow = bandRow.removeFromTop(54);
        const auto frequencyWidth = frequencyRow.getWidth() / 2;
        for (std::size_t i = 0; i < crossovers.size(); ++i)
        {
            auto cell = frequencyRow.removeFromLeft(frequencyWidth).reduced(6, 2);
            crossoverLabels[i].setBounds(cell.removeFromTop(22));
            crossovers[i].setBounds(cell);
        }
        const auto width = bandRow.getWidth() / static_cast<int>(bandDecay.size());
        for (std::size_t band = 0; band < bandDecay.size(); ++band)
        {
            auto cell = bandRow.removeFromLeft(width).reduced(6, 2);
            bandLabels[band].setBounds(cell.removeFromTop(22));
            bandDecay[band].setBounds(cell.removeFromTop(28));
            bandDampingLabels[band].setBounds(cell.removeFromTop(22));
            bandDamping[band].setBounds(cell.removeFromTop(28));
            bandLevelLabels[band].setBounds(cell.removeFromTop(22));
            bandLevel[band].setBounds(cell.removeFromTop(28));
            bandSendLabels[band].setBounds(cell.removeFromTop(22));
            bandSend[band].setBounds(cell.removeFromTop(28));
            bandModLabels[band].setBounds(cell.removeFromTop(22));
            bandMod[band].setBounds(cell);
        }
    }
    if (bloom.getToggleState())
    {
        sectionBounds[3] = controls.removeFromTop(sectionHeight);
        layoutRow(sectionBounds[3].reduced(12, 10), bloomStart, swellStart - bloomStart);
    }
    if (swell.getToggleState())
    {
        sectionBounds[4] = controls.removeFromTop(sectionHeight);
        layoutRow(sectionBounds[4].reduced(12, 10), swellStart, sliders.size() - swellStart);
    }
    if (duckPanel.getToggleState())
    {
        sectionBounds[6] = controls.removeFromTop(duckPanelHeight);
        auto row = sectionBounds[6].reduced(12, 14);
        duckOn.setBounds(row.removeFromTop(28).removeFromLeft(125));
        const auto width = row.getWidth() / static_cast<int>(duckSliders.size());
        for (std::size_t i = 0; i < duckSliders.size(); ++i)
        {
            auto cell = row.removeFromLeft(width).reduced(5, 0);
            duckLabels[i].setBounds(cell.removeFromTop(22));
            duckSliders[i].setBounds(cell);
        }
    }
    if (shimmerPanel.getToggleState())
    {
        const auto shimmerTop = controls.getY();
        shimmerSignalPath.setBounds(controls.removeFromTop(32).reduced(8, 0));
        auto globals = controls.removeFromTop(100).reduced(14, 8);
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
        sectionBounds[5] = { controls.getX(), shimmerTop, controls.getWidth(),
                             controls.getY() - shimmerTop + shimmerBottomPadding };
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
