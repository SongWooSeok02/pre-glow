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
    const auto legacyMix = mix->load(std::memory_order_relaxed);
    return { decay->load(std::memory_order_relaxed), damping->load(std::memory_order_relaxed),
             wet->load(std::memory_order_relaxed) * 0.02f * legacyMix, earlyWidth->load(std::memory_order_relaxed) * 0.01f,
             lateWidth->load(std::memory_order_relaxed) * 0.01f,
             dry->load(std::memory_order_relaxed) * 0.02f * (1.0f - legacyMix),
             freeze->load(std::memory_order_relaxed) >= 0.5f };
}

void PluginProcessor::prepareToPlay(double sampleRate, int)
{
    const auto settings = readSettings();
    for (std::size_t i = 0; i < engines.size(); ++i)
    {
        engines[i].prepare(sampleRate, static_cast<ReverbCharacter>(i));
        engines[i].setTarget(settings);
    }
    characterSmoothing = static_cast<float>(-std::expm1(-1.0 / (0.025 * sampleRate)));
    reset();
}

std::size_t PluginProcessor::readCharacter() const noexcept
{
    const auto value = character->load(std::memory_order_relaxed);
    return std::isfinite(value) ? static_cast<std::size_t>(std::clamp(value, 0.0f, static_cast<float>(characterCount - 1))) : 0;
}

void PluginProcessor::reset()
{
    for (auto& engine : engines)
        engine.reset();
    targetCharacter = readCharacter();
    characterWeights.fill(0.0f);
    characterWeights[targetCharacter] = 1.0f;
}

void PluginProcessor::processSample(float& left, float& right) noexcept
{
    float outputLeft = 0.0f;
    float outputRight = 0.0f;
    for (std::size_t i = 0; i < engines.size(); ++i)
    {
        auto l = left;
        auto r = right;
        engines[i].process(l, r);
        auto& weight = characterWeights[i];
        const auto target = i == targetCharacter ? 1.0f : 0.0f;
        weight += characterSmoothing * (target - weight);
        outputLeft += weight * l;
        outputRight += weight * r;
    }
    left = outputLeft;
    right = outputRight;
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    const auto output = layout.getMainOutputChannelSet();
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    return (input == mono && (output == mono || output == stereo))
        || (input == stereo && output == stereo);
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    upmixMonoInput(buffer);
    const auto settings = readSettings();
    targetCharacter = readCharacter();
    for (auto& engine : engines)
        engine.setTarget(settings);
    auto* left = buffer.getWritePointer(0);
    if (getMainBusNumOutputChannels() == 1)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto right = left[sample];
            processSample(left[sample], right);
            left[sample] = 0.5f * (left[sample] + right);
        }
        return;
    }

    auto* right = buffer.getWritePointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        processSample(left[sample], right[sample]);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    upmixMonoInput(buffer);
}

void PluginProcessor::upmixMonoInput(juce::AudioBuffer<float>& buffer) const noexcept
{
    if (getMainBusNumInputChannels() == 1 && getMainBusNumOutputChannels() == 2)
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
        && (xml->getIntAttribute("schemaVersion") == 1
            || xml->getIntAttribute("schemaVersion") == parameters::schemaVersion))
    {
        auto restored = juce::ValueTree::fromXml(*xml);
        const auto legacy = xml->getIntAttribute("schemaVersion") == 1;
        auto mixNode = restored.getChildWithProperty("id", parameters::mix);
        const auto storedMix = static_cast<float>(mixNode.getProperty("value", 0.35f));
        const auto oldWet = std::isfinite(storedMix) ? std::clamp(storedMix, 0.0f, 1.0f) : 0.35f;
        if (legacy && mixNode.isValid())
            mixNode.setProperty("value", 0.5f, nullptr);
        for (const auto& [id, value] : { std::pair { parameters::character, 0.0f },
                                         std::pair { parameters::earlyWidth, 100.0f },
                                         std::pair { parameters::lateWidth, 100.0f },
                                         std::pair { parameters::freeze, 0.0f },
                                         std::pair { parameters::mix, 0.5f },
                                         std::pair { parameters::dry, legacy ? 100.0f * (1.0f - oldWet) : 65.0f },
                                         std::pair { parameters::wet, legacy ? 100.0f * oldWet : 35.0f } })
        {
            if (restored.getChildWithProperty("id", id).isValid())
                continue;
            juce::ValueTree parameter { "PARAM" };
            parameter.setProperty("id", id, nullptr);
            parameter.setProperty("value", value, nullptr);
            restored.appendChild(parameter, nullptr);
        }
        restored.setProperty("schemaVersion", parameters::schemaVersion, nullptr);
        state.replaceState(restored);
    }
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new preglow::PluginProcessor();
}
