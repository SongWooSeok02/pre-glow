#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
#include <limits>

namespace preglow
{
PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, parameters::stateType, parameters::createLayout())
{
    for (std::size_t i = 0; i < duckParameters.size(); ++i)
        duckParameters[i] = state.getRawParameterValue(parameters::duckIds[i]);
    for (std::size_t i = 0; i < crossoverParameters.size(); ++i)
        crossoverParameters[i] = state.getRawParameterValue(parameters::crossoverIds[i]);
    for (std::size_t band = 0; band < bandDecayParameters.size(); ++band)
    {
        bandDecayParameters[band] = state.getRawParameterValue(parameters::bandDecayIds[band]);
        bandModParameters[band] = state.getRawParameterValue(parameters::bandModIds[band]);
        bandSendParameters[band] = state.getRawParameterValue(parameters::bandSendIds[band]);
        bandLevelParameters[band] = state.getRawParameterValue(parameters::bandLevelIds[band]);
        bandDampingParameters[band] = state.getRawParameterValue(parameters::bandDampingIds[band]);
    }
    for (std::size_t voice = 0; voice < shimmerVoices.size(); ++voice)
    {
        voiceEnabled[voice] = state.getRawParameterValue(parameters::voiceId(voice, parameters::voiceEnabled));
        for (std::size_t field = 0; field < parameters::voiceParameters.size(); ++field)
            shimmerVoices[voice][field] = state.getRawParameterValue(parameters::voiceId(voice, parameters::voiceParameters[field]));
    }
}

void PluginProcessor::updateBandDecay() noexcept
{
    std::array<float, 3> amounts {}, dampingAmounts {}, levels {}, sends {}, modulation {};
    for (std::size_t band = 0; band < amounts.size(); ++band)
    {
        const auto value = bandDecayParameters[band]->load(std::memory_order_relaxed);
        const auto depth = bandModParameters[band]->load(std::memory_order_relaxed);
        modulation[band] = std::isfinite(depth) ? std::clamp(depth, 0.0f, 100.0f) * 0.01f : 0.0f;
        const auto send = bandSendParameters[band]->load(std::memory_order_relaxed);
        sends[band] = std::isfinite(send) ? std::clamp(send, 0.0f, 100.0f) * 0.01f : 1.0f;
        const auto level = bandLevelParameters[band]->load(std::memory_order_relaxed);
        levels[band] = std::isfinite(level) ? std::clamp(level, 0.0f, 200.0f) * 0.01f : 1.0f;
        const auto dampingValue = bandDampingParameters[band]->load(std::memory_order_relaxed);
        dampingAmounts[band] = std::isfinite(dampingValue) ? std::clamp(dampingValue, 0.0f, 100.0f) * 0.01f : 0.0f;
        amounts[band] = std::isfinite(value) ? std::clamp(value, 0.25f, 2.0f) : 1.0f;
    }
    const auto lowHz = crossoverParameters[0]->load(std::memory_order_relaxed);
    const auto highHz = crossoverParameters[1]->load(std::memory_order_relaxed);
    for (auto* bank : { &engines, &liveEngines })
        for (auto& engine : *bank)
        {
            engine.setBandDecay(amounts);
            engine.setBandDamping(dampingAmounts);
            engine.setBandLevel(levels);
            engine.setBandSend(sends);
            engine.setBandModulation(modulation);
            engine.setBandCrossovers(std::isfinite(lowHz) ? lowHz : 250.0f,
                                     std::isfinite(highHz) ? highHz : 4000.0f);
        }
}

void PluginProcessor::updateLowCut() noexcept
{
    const auto placement = filterPlacement->load(std::memory_order_relaxed);
    const auto cutoff = lowCut->load(std::memory_order_relaxed);
    for (auto* bank : { &engines, &liveEngines })
        for (auto& engine : *bank)
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
    for (auto* bank : { &engines, &liveEngines })
        for (auto& engine : *bank)
            engine.setShimmer(voices, amount, feedback, route >= 1.5f && route < 2.5f, route >= 2.5f);
}

static float morphValue(float base, float axis) noexcept
{
    if (!std::isfinite(axis))
        return base;
    axis = std::clamp(axis, -1.0f, 1.0f);
    return base + axis * (axis < 0.0f ? base : 1.0f - base);
}

float PluginProcessor::effectiveBlend() const noexcept
{
    const auto x = morphX->load(std::memory_order_relaxed);
    return std::isfinite(x) ? 0.5f * (1.0f + std::clamp(x, -1.0f, 1.0f)) : 0.5f;
}

float PluginProcessor::effectiveWet() const noexcept
{
    const auto y = morphY->load(std::memory_order_relaxed);
    return std::isfinite(y) ? 0.5f * (1.0f + std::clamp(y, -1.0f, 1.0f)) : 0.35f;
}

ReverbSettings PluginProcessor::readSettings() const noexcept
{
    const auto legacyMix = mix->load(std::memory_order_relaxed);
    return { decay->load(std::memory_order_relaxed), damping->load(std::memory_order_relaxed),
             effectiveWet() * 2.0f * legacyMix, earlyWidth->load(std::memory_order_relaxed) * 0.01f,
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
    meterReleasePerSample = static_cast<float>(std::log(0.1) / (0.6 * sampleRate));
    for (auto& peak : meterPeaks)
        peak.store(0.0f, std::memory_order_relaxed);
    const auto settings = readSettings();
    auto liveSettings = settings;
    liveSettings.freeze = false;
    for (std::size_t i = 0; i < engines.size(); ++i)
    {
        engines[i].prepare(sampleRate, static_cast<ReverbCharacter>(i));
        engines[i].setTarget(settings);
        liveEngines[i].prepare(sampleRate, static_cast<ReverbCharacter>(i));
        liveEngines[i].setTarget(liveSettings);
    }
    spectrumRate = sampleRate;
    reverseFastRelease = static_cast<float>(-std::expm1(-1.0 / (0.006 * sampleRate)));
    reverseSlowRate = static_cast<float>(-std::expm1(-1.0 / (0.032 * sampleRate)));
    reverseGainRate = static_cast<float>(-std::expm1(-1.0 / (0.003 * sampleRate)));
    reverseHoldoffSamples = std::max(1, static_cast<int>(0.025 * sampleRate));
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
    const auto blend = effectiveBlend();
    targetCharacterWeights.fill(0.0f);
    targetCharacterWeights[readIndex(*character)] = 1.0f - blend;
    targetCharacterWeights[readIndex(*characterB)] += blend;
}

void PluginProcessor::reset()
{
    for (auto& engine : engines)
        engine.reset();
    for (auto& engine : liveEngines)
        engine.reset();
    livePathStarted = false;
    duckGain = 1.0f;
    reverseMix = reverseTarget = reverseFast = reverseSlow = 0.0f;
    reverseGain = 1.0f;
    reverseHoldoff = 0;
    reversePhase = 1.0f;
    reverseOnsetReady = true;
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
        auto output = engines[i].processSources(left, right);
        if (livePathStarted)
        {
            const auto amount = engines[i].freezeAmount();
            const auto live = liveEngines[i].processSources(amount * left, amount * right);
            for (std::size_t channel = 0; channel < output.wet.size(); ++channel)
                output.wet[channel] += live.wet[channel];
        }
        auto& weight = characterWeights[i];
        const auto target = targetCharacterWeights[i];
        weight += characterSmoothing * (target - weight);
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            mixed.dry[channel] += weight * output.dry[channel];
            mixed.wet[channel] += weight * output.wet[channel];
        }
    }
    const auto peak = std::max(std::abs(left), std::abs(right));
    const auto inputLevel = std::isfinite(peak) ? peak : 0.0f;
    reverseFast = std::max(inputLevel, reverseFast * (1.0f - reverseFastRelease));
    reverseSlow += reverseSlowRate * (reverseFast - reverseSlow);
    if (reverseHoldoff > 0)
        --reverseHoldoff;
    // Detect a new attack above the recent input level, even over a ringing note.
    const auto attack = reverseFast - reverseSlow;
    if (attack < std::max(0.002f, 0.12f * reverseSlow))
        reverseOnsetReady = true;
    if (reverseOnsetReady && reverseHoldoff == 0 && reverseFast >= 0.01f
        && attack > std::max(0.007f, 0.35f * reverseSlow))
    {
        reverseOnsetReady = false;
        reverseHoldoff = reverseHoldoffSamples;
        reversePhase = 0.0f;
    }

    reversePhase = std::min(1.0f, reversePhase + reverseStep);
    reverseMix += characterSmoothing * (reverseTarget - reverseMix);
    const auto targetReverseGain = 1.0f - reverseMix * (1.0f - reversePhase * reversePhase);
    reverseGain += reverseGainRate * (targetReverseGain - reverseGain);
    const auto over = std::isfinite(peak) && peak > duckThreshold ? 1.0f - duckThreshold / peak : 0.0f;
    const auto targetDuckGain = 1.0f - duckAmount * over;
    duckGain += (targetDuckGain < duckGain ? duckAttack : duckRelease) * (targetDuckGain - duckGain);
    for (auto& channel : mixed.wet)
        channel *= duckGain * reverseGain;
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
    const auto readDuck = [this](std::size_t i)
    {
        const auto value = duckParameters[i]->load(std::memory_order_relaxed);
        return std::isfinite(value) ? std::clamp(value, parameters::duckRanges[i].start, parameters::duckRanges[i].end) : parameters::duckDefaults[i];
    };
    duckThreshold = juce::Decibels::decibelsToGain(readDuck(0));
    duckAmount = duckEnabled->load(std::memory_order_relaxed) >= 0.5f ? readDuck(1) * 0.01f : 0.0f;
    reverseTarget = reverseEnabled->load(std::memory_order_relaxed) >= 0.5f ? 1.0f : 0.0f;
    const auto rise = reverseRise->load(std::memory_order_relaxed);
    const auto riseMs = std::isfinite(rise) ? std::clamp(rise, 50.0f, 2000.0f) : 450.0f;
    reverseStep = static_cast<float>(1000.0 / (riseMs * spectrumRate));
    const auto coefficient = [this](float ms)
    {
        return static_cast<float>(-std::expm1(-1000.0 / (ms * spectrumRate)));
    };
    duckAttack = coefficient(readDuck(2));
    duckRelease = coefficient(readDuck(3));
    const auto settings = readSettings();
    auto liveSettings = settings;
    liveSettings.freeze = false;
    updateCharacterTargets();
    for (auto& engine : engines)
        engine.setTarget(settings);
    for (auto& engine : liveEngines)
        engine.setTarget(liveSettings);
    livePathStarted |= settings.freeze;
    updateShimmer();
    updateLowCut();
    updateBandDecay();
    const auto samples = buffer.getNumSamples();
    const auto inputPeak = getBusBuffer(buffer, true, 0).getMagnitude(0, samples);
    auto* left = buffer.getWritePointer(0);
    if (getMainBusNumOutputChannels() == 1)
    {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto right = left[sample];
            processSample(left[sample], right, true);
            left[sample] = 0.5f * (left[sample] + right);
        }
        publishMeterPeaks(inputPeak, buffer.getMagnitude(0, 0, samples), samples);
        return;
    }

    auto* right = buffer.getWritePointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        processSample(left[sample], right[sample], false);
    publishMeterPeaks(inputPeak, getBusBuffer(buffer, false, 0).getMagnitude(0, samples), samples);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    upmixMonoInput(buffer);
    const auto* left = buffer.getReadPointer(0);
    const auto* right = buffer.getReadPointer(getMainBusNumOutputChannels() - 1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        captureSpectrum({ { left[sample], right[sample] }, {} });
    const auto samples = buffer.getNumSamples();
    const auto peak = getBusBuffer(buffer, true, 0).getMagnitude(0, samples);
    publishMeterPeaks(peak, peak, samples);
}

void PluginProcessor::publishMeterPeaks(float input, float output, int samples) noexcept
{
    const auto release = std::exp(meterReleasePerSample * static_cast<float>(samples));
    const std::array peaks { input, output };
    for (std::size_t i = 0; i < peaks.size(); ++i)
        meterPeaks[i].store(std::max(peaks[i], meterPeaks[i].load(std::memory_order_relaxed) * release),
                            std::memory_order_relaxed);
    meterRevision.fetch_add(1, std::memory_order_release);
}

std::array<float, 2> PluginProcessor::readMeterPeaks(unsigned& revision) const noexcept
{
    revision = meterRevision.load(std::memory_order_acquire);
    return { meterPeaks[0].load(std::memory_order_relaxed), meterPeaks[1].load(std::memory_order_relaxed) };
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
    restoreState(data, size, false);
}

bool PluginProcessor::loadPreset(const juce::MemoryBlock& data)
{
    const auto size = data.getSize();
    if (size < 9 || size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    const auto* bytes = static_cast<const char*>(data.getData());
    if (juce::ByteOrder::littleEndianInt(bytes + 4) != size - 9 || bytes[size - 1] != 0)
        return false;
    return restoreState(bytes, static_cast<int>(size), true);
}

void PluginProcessor::loadFactoryPreset(int characterIndex, float decaySeconds, float wetPercent)
{
    juce::ValueTree snapshot { parameters::stateType };
    snapshot.setProperty("schemaVersion", parameters::schemaVersion, nullptr);
    for (const auto* parameter : getParameters())
    {
        const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*>(parameter);
        if (ranged == nullptr)
            continue;
        const auto id = ranged->getParameterID();
        auto value = ranged->convertFrom0to1(ranged->getDefaultValue());
        if (id == parameters::character)
            value = static_cast<float>(characterIndex);
        else if (id == parameters::decay)
            value = decaySeconds;
        else if (id == parameters::morphY)
            value = wetPercent * 0.02f - 1.0f;
        else if (id == parameters::dry)
            value = 100.0f - wetPercent;
        juce::ValueTree node { "PARAM" };
        node.setProperty("id", id, nullptr);
        node.setProperty("value", value, nullptr);
        snapshot.appendChild(node, nullptr);
    }
    juce::MemoryBlock data;
    copyXmlToBinary(*snapshot.createXml(), data);
    loadPreset(data);
}

bool PluginProcessor::restoreState(const void* data, int size, bool validatePreset)
{
    const auto xml = getXmlFromBinary(data, size);
    if (xml != nullptr && xml->hasTagName(parameters::stateType)
        && (xml->getIntAttribute("schemaVersion") == 1
            || xml->getIntAttribute("schemaVersion") == 2
            || xml->getIntAttribute("schemaVersion") == 3
            || xml->getIntAttribute("schemaVersion") == 4
            || xml->getIntAttribute("schemaVersion") == 5
            || xml->getIntAttribute("schemaVersion") == 6
            || xml->getIntAttribute("schemaVersion") == 7
            || xml->getIntAttribute("schemaVersion") == parameters::schemaVersion))
    {
        auto restored = juce::ValueTree::fromXml(*xml);
        if (validatePreset)
        {
            if (restored.getNumChildren() == 0)
                return false;
            juce::StringArray seen;
            for (const auto child : restored)
            {
                const auto id = child.getProperty("id").toString();
                const auto value = child.getProperty("value").toString().toStdString();
                char* end = nullptr;
                const auto number = std::strtod(value.c_str(), &end);
                if (!child.hasType("PARAM") || id.isEmpty() || seen.contains(id)
                    || end == value.c_str() || *end != '\0' || !std::isfinite(number)
                    || std::abs(number) > std::numeric_limits<float>::max())
                    return false;
                seen.add(id);
            }
            if (xml->getIntAttribute("schemaVersion") == parameters::schemaVersion)
                for (const auto* parameter : getParameters())
                    if (const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*>(parameter);
                        ranged != nullptr && !seen.contains(ranged->getParameterID()))
                        return false;
        }
        const auto oldY = static_cast<float>(restored.getChildWithProperty("id", parameters::morphY).getProperty("value", 0.0f));
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
        if (xml->getIntAttribute("schemaVersion") < 7)
        {
            auto x = restored.getChildWithProperty("id", parameters::morphX);
            const auto blend = restored.getChildWithProperty("id", parameters::characterBlend);
            const auto base = std::clamp(static_cast<float>(blend.getProperty("value", 0.0f)) * 0.01f, 0.0f, 1.0f);
            // Missing axes have already been filled with the new default. Legacy axes defaulted to zero.
            const auto original = juce::ValueTree::fromXml(*xml).getChildWithProperty("id", parameters::morphX);
            x.setProperty("value", 2.0f * morphValue(base, static_cast<float>(original.getProperty("value", 0.0f))) - 1.0f, nullptr);
        }
        if (xml->getIntAttribute("schemaVersion") < 8)
        {
            auto y = restored.getChildWithProperty("id", parameters::morphY);
            const auto wetNode = restored.getChildWithProperty("id", parameters::wet);
            const auto savedWet = static_cast<float>(wetNode.getProperty("value", 35.0f));
            const auto base = std::isfinite(savedWet) ? std::clamp(savedWet * 0.01f, 0.0f, 1.0f) : 0.35f;
            y.setProperty("value", 2.0f * morphValue(base, oldY) - 1.0f, nullptr);
        }
        restored.setProperty("schemaVersion", parameters::schemaVersion, nullptr);
        state.replaceState(restored);
        return true;
    }
    return false;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new preglow::PluginProcessor();
}
