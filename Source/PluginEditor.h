#pragma once
#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>

namespace preglow
{
class PluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor(PluginProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class MorphPad final : public juce::Component
    {
    public:
        explicit MorphPad(PluginProcessor&);
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseDoubleClick(const juce::MouseEvent&) override;

    private:
        juce::Rectangle<float> plotBounds() const;
        juce::Point<float> position;
        juce::ParameterAttachment xAttachment, yAttachment;
    };

    void bindMorphSlider(juce::Slider&, const char* axisId,
                         std::unique_ptr<juce::ParameterAttachment>&);
    void updateMorphControls();
    void updateSections();
    void configurePresets();
    void refreshPresets();
    void loadSelectedPreset();
    void editPresetName(bool rename);
    void deleteSelectedPreset();
    void showPresetStatus(const juce::String&);
    juce::File selectedUserPreset() const;
    juce::TextButton presetButton { "Presets" };
    juce::Component presetPanel;
    juce::TextEditor presetSearch;
    juce::ComboBox presetCategory, presetChoices;
    juce::TextButton presetLoad { "Load" }, presetSave { "Save As" },
                     presetRename { "Rename" }, presetDelete { "Delete" };
    juce::Label presetStatus;
    juce::Array<juce::File> listedUserPresets;
    juce::TextButton advancedMode { "Basic / Advanced" };
    juce::TextButton morphPanel { "Show XY Morph" };
    juce::ToggleButton duckOn { "Ducking On" }, reverseOn { "Reverse-like" };
    juce::Slider reverseTime;
    juce::Label reverseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> duckOnBinding, reverseOnBinding;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverseTimeBinding;
    juce::TextButton duckPanel { "Ducking" };
    std::array<juce::Slider, 4> duckSliders;
    std::array<juce::Label, 4> duckLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> duckAttachments;
    juce::Label morphReadout;
    MorphPad morphPad;
    void timerCallback() override;
    void paintSpectrum(juce::Graphics&);
    PluginProcessor& ownerProcessor;
    void paintMeters(juce::Graphics&);
    juce::Rectangle<int> spectrumBounds, meterBounds;
    std::array<float, 2> displayedPeaks {};
    unsigned lastMeterRevision = 0;
    double lastMeterUpdate = 0.0;
    juce::dsp::FFT spectrumFFT { PluginProcessor::spectrumOrder };
    juce::dsp::WindowingFunction<float> spectrumWindow {
        PluginProcessor::spectrumSize, juce::dsp::WindowingFunction<float>::hann };
    PluginProcessor::SpectrumFrame spectrumFrame {};
    std::array<float, 2 * PluginProcessor::spectrumSize> fftWork {};
    std::array<std::array<float, PluginProcessor::spectrumSize / 2 + 1>, 2> spectrum {};
    double spectrumSampleRate = 0.0;
    juce::TextButton shimmerPanel { "Shimmer" };
    juce::ToggleButton shimmerPower { "Shimmer On" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shimmerPowerAttachment;
    std::array<juce::ToggleButton, Shimmer::voiceCount> voiceToggles;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, Shimmer::voiceCount> voiceToggleAttachments;
    juce::ComboBox shimmerRouting;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shimmerRoutingAttachment;
    juce::Component voiceContent;
    std::array<juce::GroupComponent, Shimmer::voiceCount> voiceCards;
    std::array<juce::TextButton, Shimmer::voiceCount> voiceDetails;
    juce::Viewport voiceViewport;
    std::array<juce::Slider, 2> shimmerGlobals;
    std::array<juce::Label, 2> shimmerGlobalLabels;
    std::array<juce::Slider, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceSliders;
    std::array<juce::Label, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceLabels;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 2> shimmerAttachments;
    std::array<std::unique_ptr<SliderAttachment>, Shimmer::voiceCount * parameters::voiceParameters.size()> voiceAttachments;
    juce::ToggleButton bloom { "Bloom" }, swell { "Swell" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bloomAttachment, swellAttachment;
    juce::ToggleButton infinite { "Infinite Decay" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> infiniteAttachment;
    juce::ToggleButton freeze { "Freeze" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    std::array<juce::Rectangle<int>, 7> sectionBounds;
    std::array<juce::Slider, 2> crossovers;
    std::array<juce::Label, 2> crossoverLabels;
    std::array<std::unique_ptr<SliderAttachment>, 2> crossoverAttachments;
    juce::TextButton bandPanel { "Multiband" };
    std::array<juce::Slider, 3> bandDecay, bandDamping, bandLevel, bandSend, bandMod;
    std::array<juce::Label, 3> bandLabels, bandDampingLabels, bandLevelLabels, bandSendLabels, bandModLabels;
    std::array<std::unique_ptr<SliderAttachment>, 3> bandAttachments, bandDampingAttachments, bandLevelAttachments, bandSendAttachments, bandModAttachments;
    juce::ComboBox filterPlacement;
    juce::Slider lowCut;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterPlacementAttachment;
    std::unique_ptr<SliderAttachment> lowCutAttachment;
    juce::ComboBox character, characterB;
    juce::Slider characterBlend;
    juce::Label characterALabel, characterBLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterBAttachment;
    std::unique_ptr<juce::ParameterAttachment> characterBlendAttachment, wetMorphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment;
    std::array<juce::Slider, 13> sliders;
    std::array<juce::Label, 13> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 13> attachments;
};
}
