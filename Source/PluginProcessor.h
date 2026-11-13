#pragma once
#include "Parameters.h"
#include "../DSP/Reverb/Reverb.h"

namespace preglow
{
class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
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
    double getTailLengthSeconds() const override { return 120.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState state;

private:
    void upmixMonoInput(juce::AudioBuffer<float>&) const noexcept;
    ReverbSettings readSettings() const noexcept;
    const std::atomic<float>* const decay = state.getRawParameterValue(parameters::decay);
    const std::atomic<float>* const damping = state.getRawParameterValue(parameters::damping);
    const std::atomic<float>* const mix = state.getRawParameterValue(parameters::mix);
    void processSample(float& left, float& right) noexcept;
    std::size_t readCharacter() const noexcept;
    const std::atomic<float>* const character = state.getRawParameterValue(parameters::character);
    std::array<Reverb, 3> engines;
    std::array<float, 3> characterWeights { 1.0f, 0.0f, 0.0f };
    std::size_t targetCharacter = 0;
    float characterSmoothing = 1.0f;
};
}
