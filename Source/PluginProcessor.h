#pragma once
#include "Parameters.h"
#include "../DSP/Reverb/Reverb.h"

namespace preglow
{
class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    float effectiveBlend() const noexcept;
    float effectiveWet() const noexcept;
    void prepareToPlay(double sampleRate, int maxBlockSize) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    using AudioProcessor::processBlockBypassed;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return infinite->load(std::memory_order_relaxed) >= 0.5f ? Reverb::infiniteDecaySeconds : 120.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool loadPreset(const juce::MemoryBlock&);
    void loadFactoryPreset(int characterIndex, float decaySeconds, float wetPercent);

    static constexpr int spectrumOrder = 11;
    static constexpr int spectrumSize = 1 << spectrumOrder;
    using SpectrumFrame = std::array<std::array<float, spectrumSize>, 4>;
    bool readSpectrum(SpectrumFrame& frame, double& sampleRate) noexcept;

    std::array<float, 4> readMeterPeaks(unsigned& revision) const noexcept;

    juce::AudioProcessorValueTreeState state;

private:
    void publishMeterPeaks(float input, float output, float wet, int samples) noexcept;
    static_assert(std::atomic<float>::is_always_lock_free);
    void publishCorrelation(const juce::AudioBuffer<float>&) noexcept;
    std::atomic<float> correlation { 2.0f };
    float blockWetPeak = 0.0f;
    std::array<std::atomic<float>, 3> meterPeaks {};
    std::atomic<unsigned> meterRevision { 0 };
    float meterReleasePerSample = 0.0f;
    bool restoreState(const void*, int, bool validatePreset);
    std::array<const std::atomic<float>*, Shimmer::voiceCount> voiceEnabled {};
    const std::atomic<float>* const shimmerEnabled = state.getRawParameterValue(parameters::shimmerEnabled);
    const std::atomic<float>* const shimmerRouting = state.getRawParameterValue(parameters::shimmerRouting);
    std::array<const std::atomic<float>*, 3> bandDecayParameters {}, bandDampingParameters {}, bandLevelParameters {}, bandSendParameters {}, bandModParameters {};
    std::array<const std::atomic<float>*, 2> crossoverParameters {};
    void updateBandDecay() noexcept;
    void updateLowCut() noexcept;
    const std::atomic<float>* const filterPlacement = state.getRawParameterValue(parameters::filterPlacement);
    const std::atomic<float>* const lowCut = state.getRawParameterValue(parameters::lowCut);
    void updateShimmer() noexcept;
    std::array<std::array<const std::atomic<float>*, parameters::voiceParameters.size()>, Shimmer::voiceCount> shimmerVoices {};
    const std::atomic<float>* const shimmerMix = state.getRawParameterValue(parameters::shimmerMix);
    const std::atomic<float>* const shimmerFeedback = state.getRawParameterValue(parameters::shimmerFeedback);
    void upmixMonoInput(juce::AudioBuffer<float>&) const noexcept;
    const std::atomic<float>* const morphX = state.getRawParameterValue(parameters::morphX);
    const std::atomic<float>* const morphY = state.getRawParameterValue(parameters::morphY);
    std::array<const std::atomic<float>*, 4> duckParameters {};
    const std::atomic<float>* const duckEnabled = state.getRawParameterValue(parameters::duckEnabled);
    const std::atomic<float>* const reverseEnabled = state.getRawParameterValue(parameters::reverseEnabled);
    const std::atomic<float>* const reverseRise = state.getRawParameterValue(parameters::reverseRise);
    float reverseMix = 0.0f, reverseTarget = 0.0f, reversePhase = 1.0f, reverseStep = 1.0f;
    float reverseFast = 0.0f, reverseSlow = 0.0f, reverseGain = 1.0f;
    float reverseFastRelease = 0.0f, reverseSlowRate = 0.0f, reverseGainRate = 0.0f;
    int reverseHoldoff = 0, reverseHoldoffSamples = 1;
    bool reverseOnsetReady = true;
    float duckGain = 1.0f, duckAmount = 0.0f, duckThreshold = 1.0f;
    float duckAttack = 1.0f, duckRelease = 1.0f;
    ReverbSettings readSettings() const noexcept;
    const std::atomic<float>* const decay = state.getRawParameterValue(parameters::decay);
    const std::atomic<float>* const damping = state.getRawParameterValue(parameters::damping);
    const std::atomic<float>* const mix = state.getRawParameterValue(parameters::mix);
    const std::atomic<float>* const dry = state.getRawParameterValue(parameters::dry);
    void processSample(float& left, float& right, bool mono) noexcept;
    void captureSpectrum(const Reverb::Output& output) noexcept;
    SpectrumFrame spectrumWrite {}, spectrumReady {};
    std::atomic<bool> spectrumPending { false };
    std::size_t spectrumPosition = 0;
    double spectrumRate = 0.0, readySpectrumRate = 0.0;
    void updateCharacterTargets() noexcept;
    const std::atomic<float>* const character = state.getRawParameterValue(parameters::character);
    const std::atomic<float>* const earlyWidth = state.getRawParameterValue(parameters::earlyWidth);
    const std::atomic<float>* const bloomEnabled = state.getRawParameterValue(parameters::bloomEnabled);
    const std::atomic<float>* const swellEnabled = state.getRawParameterValue(parameters::swellEnabled);
    const std::atomic<float>* const swellTime = state.getRawParameterValue(parameters::swellTime);
    const std::atomic<float>* const swellSensitivity = state.getRawParameterValue(parameters::swellSensitivity);
    const std::atomic<float>* const swellCurve = state.getRawParameterValue(parameters::swellCurve);
    const std::atomic<float>* const bloomAmount = state.getRawParameterValue(parameters::bloomAmount);
    const std::atomic<float>* const bloomAttack = state.getRawParameterValue(parameters::bloomAttack);
    const std::atomic<float>* const bloomRelease = state.getRawParameterValue(parameters::bloomRelease);
    const std::atomic<float>* const bloomShape = state.getRawParameterValue(parameters::bloomShape);
    const std::atomic<float>* const infinite = state.getRawParameterValue(parameters::infinite);
    const std::atomic<float>* const freeze = state.getRawParameterValue(parameters::freeze);
    const std::atomic<float>* const lateWidth = state.getRawParameterValue(parameters::lateWidth);
    static constexpr auto characterCount = static_cast<std::size_t>(ReverbCharacter::count);
    std::array<Reverb, characterCount> engines, liveEngines;
    bool livePathStarted = false;
    std::array<float, characterCount> characterWeights { 1.0f };
    const std::atomic<float>* const characterB = state.getRawParameterValue(parameters::characterB);
    std::array<float, characterCount> targetCharacterWeights { 1.0f };
    float characterSmoothing = 1.0f;
};
}
