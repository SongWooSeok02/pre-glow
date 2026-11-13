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
    void reset() override { reverb.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "pre-glow"; }
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
    ReverbSettings readSettings() const noexcept;
    const std::atomic<float>* const decay = state.getRawParameterValue(parameters::decay);
    const std::atomic<float>* const damping = state.getRawParameterValue(parameters::damping);
    const std::atomic<float>* const mix = state.getRawParameterValue(parameters::mix);
    Reverb reverb;
};
}
