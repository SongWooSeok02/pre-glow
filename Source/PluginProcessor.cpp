#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace preglow
{
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, parameters::stateType, parameters::createLayout())
{
    for (std::size_t i = 0; i < crossoverParameters.size(); ++i)
        crossoverParameters[i] = state.getRawParameterValue(parameters::crossoverIds[i]);
    for (std::size_t band = 0; band < bandDecayParameters.size(); ++band)
        bandDecayParameters[band] = state.getRawParameterValue(parameters::bandDecayIds[band]);
    for (std::size_t voice = 0; voice < shimmerVoices.size(); ++voice)
    {
        voiceEnabled[voice] = state.getRawParameterValue(parameters::voiceId(voice, parameters::voiceEnabled));
        for (std::size_t field = 0; field < parameters::voiceParameters.size(); ++field)
            shimmerVoices[voice][field] = state.getRawParameterValue(parameters::voiceId(voice, parameters::voiceParameters[field]));
    }
}

void PluginProcessor::updateBandDecay() noexcept
{
    std::array<float, 3> amounts {};
    for (std::size_t band = 0; band < amounts.size(); ++band)
    {
        const auto value = bandDecayParameters[band]->load(std::memory_order_relaxed);
        amounts[band] = std::isfinite(value) ? std::clamp(value, 0.25f, 2.0f) : 1.0f;
    }
    const auto lowHz = crossoverParameters[0]->load(std::memory_order_relaxed);
    const auto highHz = crossoverParameters[1]->load(std::memory_order_relaxed);
    for (auto& engine : engines)
    {
        engine.setBandDecay(amounts);
        engine.setBandCrossovers(std::isfinite(lowHz) ? lowHz : 250.0f,
                                 std::isfinite(highHz) ? highHz : 4000.0f);
    }
}

void PluginProcessor::updateLowCut() noexcept
{
    const auto placement = filterPlacement->load(std::memory_order_relaxed);
    const auto cutoff = lowCut->load(std::memory_order_relaxed);
    for (auto& engine : engines)
        engine.setLowCut(placement >= 1.5f ? 2 : placement >= 0.5f ? 1 : 0,
                         std::isfinite(cutoff) ? std::clamp(cutoff, 20.0f, 2000.0f) : 120.0f);
}

void PluginProcessor::updateShimmer() noexcept
{
    std::array<Shimmer::Voice, Shimmer::voiceCount> voices;
    for (std::size_t i = 0; i < voices.size(); ++i)
    {
        const auto& fields = shimmerVoices[i];
        const auto load = [&](std::size_t field) { return fields[field]->load(std::memory_order_relaxed); };
        voices[i] = { load(0), load(1), voiceEnabled[i]->load(std::memory_order_relaxed) >= 0.5f ? load(2) : 0.0f, load(3), load(4), load(5), load(6) };
    }
    // The reverb smooths the effective mix for both the wet layer and feedback send.
    const auto amount = shimmerEnabled->load(std::memory_order_relaxed) > 0.5f
        ? shimmerMix->load(std::memory_order_relaxed) * 0.01f : 0.0f;
    const auto route = shimmerRouting->load(std::memory_order_relaxed);
    const auto feedback = route < 0.5f
        ? shimmerFeedback->load(std::memory_order_relaxed) * 0.01f : 0.0f;
    for (auto& engine : engines)
        engine.setShimmer(voices, amount, feedback, route >= 1.5f && route < 2.5f, route >= 2.5f);
}

ReverbSettings PluginProcessor::readSettings() const noexcept
{
    const auto legacyMix = mix->load(std::memory_order_relaxed);
    return { decay->load(std::memory_order_relaxed), damping->load(std::memory_order_relaxed),
             wet->load(std::memory_order_relaxed) * 0.02f * legacyMix, earlyWidth->load(std::memory_order_relaxed) * 0.01f,
             lateWidth->load(std::memory_order_relaxed) * 0.01f,
             dry->load(std::memory_order_relaxed) * 0.02f * (1.0f - legacyMix),
             freeze->load(std::memory_order_relaxed) >= 0.5f, infinite->load(std::memory_order_relaxed) >= 0.5f,
             bloomEnabled->load(std::memory_order_relaxed) >= 0.5f ? bloomAmount->load(std::memory_order_relaxed) * 0.01f : 0.0f,
             bloomAttack->load(std::memory_order_relaxed), bloomRelease->load(std::memory_order_relaxed),
             bloomShape->load(std::memory_order_relaxed) * 0.01f,
             swellEnabled->load(std::memory_order_relaxed) >= 0.5f, swellTime->load(std::memory_order_relaxed),
             swellSensitivity->load(std::memory_order_relaxed) * 0.01f, swellCurve->load(std::memory_order_relaxed) * 0.01f };
}

void PluginProcessor::prepareToPlay(double sampleRate, int)
{
    const auto settings = readSettings();
    for (std::size_t i = 0; i < engines.size(); ++i)
    {
        engines[i].prepare(sampleRate, static_cast<ReverbCharacter>(i));
        engines[i].setTarget(settings);
    }
    spectrumRate = sampleRate;
    characterSmoothing = static_cast<float>(-std::expm1(-1.0 / (0.025 * sampleRate)));
    updateShimmer();
    updateLowCut();
    updateBandDecay();
    reset();
}

void PluginProcessor::updateCharacterTargets() noexcept
{
    const auto readIndex = [](const std::atomic<float>& parameter)
    {
        const auto value = parameter.load(std::memory_order_relaxed);
        return std::isfinite(value) ? static_cast<std::size_t>(std::clamp(value, 0.0f, static_cast<float>(characterCount - 1))) : 0;
    };
    const auto blend = characterBlend->load(std::memory_order_relaxed) * 0.01f;
    targetCharacterWeights.fill(0.0f);
    targetCharacterWeights[readIndex(*character)] = 1.0f - blend;
    targetCharacterWeights[readIndex(*characterB)] += blend;
}

void PluginProcessor::reset()
{
    for (auto& engine : engines)
        engine.reset();
    spectrumPosition = 0;
    updateCharacterTargets();
    characterWeights = targetCharacterWeights;
}

bool PluginProcessor::readSpectrum(SpectrumFrame& frame, double& sampleRate) noexcept
{
    if (!spectrumPending.load(std::memory_order_acquire))
        return false;
    frame = spectrumReady;
    sampleRate = readySpectrumRate;
    spectrumPending.store(false, std::memory_order_release);
    return true;
}

void PluginProcessor::captureSpectrum(const Reverb::Output& output) noexcept
{
    spectrumWrite[0][spectrumPosition] = output.dry[0];
    spectrumWrite[1][spectrumPosition] = output.dry[1];
    spectrumWrite[2][spectrumPosition] = output.wet[0];
    spectrumWrite[3][spectrumPosition] = output.wet[1];
    if (++spectrumPosition != spectrumSize)
        return;

    spectrumPosition = 0;
    if (!spectrumPending.load(std::memory_order_acquire))
    {
        spectrumReady = spectrumWrite;
        readySpectrumRate = spectrumRate;
        spectrumPending.store(true, std::memory_order_release);
    }
}

void PluginProcessor::processSample(float& left, float& right, bool mono) noexcept
{
    Reverb::Output mixed {};
    for (std::size_t i = 0; i < engines.size(); ++i)
    {
        const auto output = engines[i].processSources(left, right);
        auto& weight = characterWeights[i];
        const auto target = targetCharacterWeights[i];
        weight += characterSmoothing * (target - weight);
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            mixed.dry[channel] += weight * output.dry[channel];
            mixed.wet[channel] += weight * output.wet[channel];
        }
    }
    left = mixed.dry[0] + mixed.wet[0];
    right = mixed.dry[1] + mixed.wet[1];
    if (mono)
    {
        mixed.dry.fill(0.5f * (mixed.dry[0] + mixed.dry[1]));
        mixed.wet.fill(0.5f * (mixed.wet[0] + mixed.wet[1]));
    }
    captureSpectrum(mixed);
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
    updateCharacterTargets();
    for (auto& engine : engines)
        engine.setTarget(settings);
    updateShimmer();
    updateLowCut();
    updateBandDecay();
    auto* left = buffer.getWritePointer(0);
    if (getMainBusNumOutputChannels() == 1)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto right = left[sample];
            processSample(left[sample], right, true);
            left[sample] = 0.5f * (left[sample] + right);
        }
        return;
    }

    auto* right = buffer.getWritePointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        processSample(left[sample], right[sample], false);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    upmixMonoInput(buffer);
    const auto* left = buffer.getReadPointer(0);
    const auto* right = buffer.getReadPointer(getMainBusNumOutputChannels() - 1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        captureSpectrum({ { left[sample], right[sample] }, {} });
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
            || xml->getIntAttribute("schemaVersion") == 2
            || xml->getIntAttribute("schemaVersion") == 3
            || xml->getIntAttribute("schemaVersion") == 4
            || xml->getIntAttribute("schemaVersion") == 5
            || xml->getIntAttribute("schemaVersion") == parameters::schemaVersion))
    {
        auto restored = juce::ValueTree::fromXml(*xml);
        const auto legacy = xml->getIntAttribute("schemaVersion") == 1;
        const auto oldBloom = restored.getChildWithProperty("id", parameters::bloomAmount);
        const auto storedBloom = static_cast<float>(oldBloom.getProperty("value", 0.0f));
        const auto legacyBloomEnabled = xml->getIntAttribute("schemaVersion") < 3
            && std::isfinite(storedBloom) && storedBloom > 0.0f;
        auto mixNode = restored.getChildWithProperty("id", parameters::mix);
        const auto storedMix = static_cast<float>(mixNode.getProperty("value", 0.35f));
        const auto oldWet = std::isfinite(storedMix) ? std::clamp(storedMix, 0.0f, 1.0f) : 0.35f;
        if (legacy && mixNode.isValid())
            mixNode.setProperty("value", 0.5f, nullptr);
        for (const auto& [id, value] : { std::pair { parameters::character, 0.0f },
                                         std::pair { parameters::earlyWidth, 100.0f },
                                         std::pair { parameters::lateWidth, 100.0f },
                                         std::pair { parameters::freeze, 0.0f },
                                         std::pair { parameters::infinite, 0.0f },
                                         std::pair { parameters::bloomAmount, 75.0f },
                                         std::pair { parameters::bloomEnabled, legacyBloomEnabled ? 1.0f : 0.0f },
                                         std::pair { parameters::swellEnabled, 0.0f },
                                         std::pair { parameters::swellTime, 400.0f },
                                         std::pair { parameters::swellSensitivity, 50.0f },
                                         std::pair { parameters::swellCurve, 50.0f },
                                         std::pair { parameters::bloomAttack, 250.0f },
                                         std::pair { parameters::bloomRelease, 1500.0f },
                                         std::pair { parameters::bloomShape, 50.0f },
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
        if (xml->getIntAttribute("schemaVersion") == 4)
            for (std::size_t voice = 0; voice < Shimmer::voiceCount; ++voice)
            {
                const auto level = restored.getChildWithProperty("id", parameters::voiceId(voice, parameters::voiceParameters[2]));
                juce::ValueTree node { "PARAM" };
                node.setProperty("id", parameters::voiceId(voice, parameters::voiceEnabled), nullptr);
                node.setProperty("value", static_cast<float>(level.getProperty("value", voice == 0 ? 1.0f : 0.0f)) > 0.0f ? 1.0f : 0.0f, nullptr);
                restored.appendChild(node, nullptr);
            }
        for (const auto* parameter : getParameters())
        {
            const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*>(parameter);
            if (ranged == nullptr || restored.getChildWithProperty("id", ranged->getParameterID()).isValid())
                continue;
            juce::ValueTree node { "PARAM" };
            node.setProperty("id", ranged->getParameterID(), nullptr);
            node.setProperty("value", ranged->convertFrom0to1(ranged->getDefaultValue()), nullptr);
            restored.appendChild(node, nullptr);
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
