#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace preglow
{
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, parameters::stateType, parameters::createLayout())
{
}

ReverbSettings PluginProcessor::readSettings() const noexcept
{
    return { decay->load(std::memory_order_relaxed), damping->load(std::memory_order_relaxed),
             mix->load(std::memory_order_relaxed) };
}

void PluginProcessor::prepareToPlay(double sampleRate, int)
{
    reverb.prepare(sampleRate);
    reverb.setTarget(readSettings());
    reverb.reset();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    upmixMonoInput(buffer);
    reverb.setTarget(readSettings());
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        reverb.process(left[sample], right[sample]);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    upmixMonoInput(buffer);
}

void PluginProcessor::upmixMonoInput(juce::AudioBuffer<float>& buffer) const noexcept
{
    if (getMainBusNumInputChannels() == 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto snapshot = state.copyState();
    snapshot.setProperty("schemaVersion", parameters::schemaVersion, nullptr);
    if (const auto xml = snapshot.createXml())
        copyXmlToBinary(*xml, destination);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    const auto xml = getXmlFromBinary(data, size);
    if (xml != nullptr && xml->hasTagName(parameters::stateType)
        && xml->getIntAttribute("schemaVersion") == parameters::schemaVersion)
        state.replaceState(juce::ValueTree::fromXml(*xml));
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new preglow::PluginProcessor();
}
