#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr size_t numBands = mx6::dsp::MultibandProcessor::numBands;

juce::String formatParameterValue(const float value)
{
    auto rounded = std::round(static_cast<double>(value) * 10.0) / 10.0;

    if (std::abs(rounded) < 0.05)
        rounded = 0.0;

    return juce::String::formatted("%+08.1f", rounded);
}

enum class ParameterType
{
    floating,
    boolean,
};

enum class ParameterSlot : size_t
{
    inGn,
    inRight,
    inLeft,
    autoInGn,
    autoInRight,
    autoInLeft,
    wide,
    thLU,
    mkLU,
    thLD,
    mkLD,
    thRU,
    mkRU,
    thRD,
    mkRD,
    hwBypass,
    LLThResh,
    LLTension,
    LLRelease,
    LLmk,
    RRThResh,
    RRTension,
    RRRelease,
    RRmk,
    DMbypass,
    FFThResh,
    FFTension,
    FFRelease,
    FFmk,
    FFbypass,
    moRph,
    peakHoldHz,
    TensionFlooR,
    TensionHysT,
    delTa,
    count
};

enum class FullbandAutomationSlot : size_t
{
    inGn,
    inRight,
    inLeft,
    count
};

enum class FullbandVisibleSlot : size_t
{
    inGn,
    outGn,
    count
};

constexpr size_t numParameterSlots = static_cast<size_t>(ParameterSlot::count);
constexpr size_t numFullbandVisibleSlots = static_cast<size_t>(FullbandVisibleSlot::count);
constexpr size_t numFullbandAutomationSlots = static_cast<size_t>(FullbandAutomationSlot::count);

struct ParameterSpec
{
    const char* suffix = "";
    const char* name = "";
    ParameterType type = ParameterType::floating;
    float min = 0.0f;
    float max = 1.0f;
    float step = 0.01f;
    float defaultValue = 0.0f;
    const char* label = "";
};

constexpr auto parameterSpecs = std::to_array<ParameterSpec>({
    { "inGn", "Input Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "inRight", "Input Right", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "inLeft", "Input Left", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "autoInGn", "AUTO INPUT-GAIN", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "autoInRight", "AUTO IN-RIGHT", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "autoInLeft", "AUTO IN-LEFT", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "wide", "Wide", ParameterType::floating, -100.0f, 400.0f, 0.1f, 0.0f, "%" },
    { "thLU", "L Up Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "mkLU", "L Up Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "thLD", "L Down Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "mkLD", "L Down Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "thRU", "R Up Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "mkRU", "R Up Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "thRD", "R Down Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "mkRD", "R Down Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "hwBypass", "HW Bypass", ParameterType::boolean, 0.0f, 1.0f, 1.0f, 1.0f, "" },
    { "LLThResh", "LL Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "LLTension", "LL Tension", ParameterType::floating, -100.0f, 100.0f, 0.1f, 0.0f, "%" },
    { "LLRelease", "LL Release", ParameterType::floating, 0.0f, 1000.0f, 0.1f, 0.0f, "ms" },
    { "LLmk", "LL Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "RRThResh", "RR Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "RRTension", "RR Tension", ParameterType::floating, -100.0f, 100.0f, 0.1f, 0.0f, "%" },
    { "RRRelease", "RR Release", ParameterType::floating, 0.0f, 1000.0f, 0.1f, 0.0f, "ms" },
    { "RRmk", "RR Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "DMbypass", "DM Bypass", ParameterType::boolean, 0.0f, 1.0f, 1.0f, 1.0f, "" },
    { "FFThResh", "FF Threshold", ParameterType::floating, -24.0f, 0.0f, 0.1f, 0.0f, "dB" },
    { "FFTension", "FF Tension", ParameterType::floating, -100.0f, 100.0f, 0.1f, 0.0f, "%" },
    { "FFRelease", "FF Release", ParameterType::floating, 0.0f, 1000.0f, 0.1f, 0.0f, "ms" },
    { "FFmk", "FF Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "FFbypass", "FF Bypass", ParameterType::boolean, 0.0f, 1.0f, 1.0f, 1.0f, "" },
    { "moRph", "Morph", ParameterType::floating, 0.0f, 100.0f, 0.1f, 0.0f, "%" },
    { "peakHoldHz", "Peak Hold", ParameterType::floating, 21.0f, 3675.1f, 0.1f, 100.0f, "Hz" },
    { "TensionFlooR", "Tension Floor", ParameterType::floating, -96.0f, 0.0f, 0.1f, -96.0f, "dB" },
    { "TensionHysT", "Tension Hysteresis", ParameterType::floating, 0.0f, 100.0f, 0.1f, 0.0f, "%" },
    { "delTa", "Delta", ParameterType::boolean, 0.0f, 1.0f, 1.0f, 0.0f, "" },
});

constexpr auto fullbandAutomationSpecs = std::to_array<ParameterSpec>({
    { "autoInGn", "ENV FULLBAND INPUT-GAIN", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "autoInRight", "ENV FULLBAND IN-RIGHT", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "autoInLeft", "ENV FULLBAND IN-LEFT", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
});

constexpr auto fullbandVisibleSpecs = std::to_array<ParameterSpec>({
    { "inGnVisible", "Fullband In Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
    { "outGnVisible", "Fullband Out Gain", ParameterType::floating, -24.0f, 24.0f, 0.1f, 0.0f, "dB" },
});

static_assert(parameterSpecs.size() == numParameterSlots);
static_assert(fullbandVisibleSpecs.size() == numFullbandVisibleSlots);
static_assert(fullbandAutomationSpecs.size() == numFullbandAutomationSlots);

constexpr size_t toIndex(const ParameterSlot slot)
{
    return static_cast<size_t>(slot);
}

constexpr bool isAutomationOnlySlot(const ParameterSlot slot)
{
    return slot == ParameterSlot::autoInGn
        || slot == ParameterSlot::autoInRight
        || slot == ParameterSlot::autoInLeft;
}

juce::String getAutomationTargetName(const ParameterSlot slot)
{
    if (slot == ParameterSlot::autoInGn)
        return "INPUT-GAIN";

    if (slot == ParameterSlot::autoInRight)
        return "IN-RIGHT";

    if (slot == ParameterSlot::autoInLeft)
        return "IN-LEFT";

    jassertfalse;
    return {};
}

juce::String makeSingleAutomationParameterName(const ParameterSlot slot)
{
    return "ENV SINGLEBAND " + getAutomationTargetName(slot);
}

juce::String makeBandAutomationParameterName(const size_t bandIndex, const ParameterSlot slot)
{
    return "ENV BAND " + juce::String(static_cast<int>(bandIndex + 1)) + " " + getAutomationTargetName(slot);
}

juce::String makeBandParameterId(const size_t bandIndex, const char* suffix)
{
    return "band" + juce::String(static_cast<int>(bandIndex + 1)) + "_" + suffix;
}

juce::String makeFullbandParameterId(const char* suffix)
{
    return "fullband_" + juce::String(suffix);
}

juce::String makeSingleParameterId(const char* suffix)
{
    return "single_" + juce::String(suffix);
}

juce::String makeBandGroupId(const size_t bandIndex)
{
    return "band" + juce::String(static_cast<int>(bandIndex + 1));
}

juce::String makeBandGroupName(const size_t bandIndex)
{
    return "Band " + juce::String(static_cast<int>(bandIndex + 1));
}

juce::String makeSingleGroupId()
{
    return "single";
}

juce::String makeSingleGroupName()
{
    return "Single";
}

juce::String makeFullbandGroupId()
{
    return "fullband";
}

juce::String makeFullbandGroupName()
{
    return "Fullband";
}

juce::String makeSoloParameterId(const size_t bandIndex)
{
    return "soloBand" + juce::String(static_cast<int>(bandIndex + 1));
}

juce::String makeSingleBandModeParameterId()
{
    return "singleBandMode";
}

juce::AudioProcessorValueTreeState::ParameterLayout buildParameterLayout()
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    using Parameter = std::unique_ptr<juce::RangedAudioParameter>;

    auto floatParam = [] (const juce::String& id,
                          const juce::String& name,
                          const float min,
                          const float max,
                          const float step,
                          const float defaultValue,
                          const juce::String& label,
                          const bool isAutomatable) -> Parameter
    {
        auto range = juce::NormalisableRange<float> { min, max, step };
        auto attributes = juce::AudioParameterFloatAttributes()
                              .withLabel(label)
                              .withAutomatable(isAutomatable)
                              .withStringFromValueFunction([] (float value, int)
                              {
                                  return formatParameterValue(value);
                              })
                              .withValueFromStringFunction([] (const juce::String& text)
                              {
                                  return text.trim().getFloatValue();
                              });
        return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, 1 }, name, range, defaultValue, attributes);
    };

    auto boolParam = [] (const juce::String& id,
                         const juce::String& name,
                         const bool defaultValue,
                         const bool isAutomatable) -> Parameter
    {
        auto attributes = juce::AudioParameterBoolAttributes()
                              .withAutomatable(isAutomatable);
        return std::make_unique<juce::AudioParameterBool>(juce::ParameterID { id, 1 }, name, defaultValue, attributes);
    };

    Layout layout;
    auto soloGroup = std::make_unique<juce::AudioProcessorParameterGroup>("monitor", "Monitor", " | ");

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
        soloGroup->addChild(boolParam(makeSoloParameterId(bandIndex), "Solo Band " + juce::String(static_cast<int>(bandIndex + 1)), false, false));

    soloGroup->addChild(boolParam(makeSingleBandModeParameterId(), "Single Band Mode", true, false));

    layout.add(std::move(soloGroup));

    auto fullbandGroup = std::make_unique<juce::AudioProcessorParameterGroup>(makeFullbandGroupId(),
                                                                              makeFullbandGroupName(),
                                                                              " | ");

    for (const auto& spec : fullbandVisibleSpecs)
        fullbandGroup->addChild(floatParam(makeFullbandParameterId(spec.suffix), spec.name, spec.min, spec.max, spec.step, spec.defaultValue, spec.label, false));

    for (const auto& spec : fullbandAutomationSpecs)
        fullbandGroup->addChild(floatParam(makeFullbandParameterId(spec.suffix), spec.name, spec.min, spec.max, spec.step, spec.defaultValue, spec.label, true));

    layout.add(std::move(fullbandGroup));

    auto singleGroup = std::make_unique<juce::AudioProcessorParameterGroup>(makeSingleGroupId(),
                                                                            makeSingleGroupName(),
                                                                            " | ");

    for (size_t parameterIndex = 0; parameterIndex < parameterSpecs.size(); ++parameterIndex)
    {
        const auto& spec = parameterSpecs[parameterIndex];
        const auto slot = static_cast<ParameterSlot>(parameterIndex);
        const auto parameterId = makeSingleParameterId(spec.suffix);
        const auto isAutomatable = isAutomationOnlySlot(slot);

        const auto resolvedParameterName = isAutomationOnlySlot(slot)
            ? makeSingleAutomationParameterName(slot)
            : juce::String(spec.name);

        if (spec.type == ParameterType::boolean)
            singleGroup->addChild(boolParam(parameterId, resolvedParameterName, spec.defaultValue >= 0.5f, isAutomatable));
        else
            singleGroup->addChild(floatParam(parameterId, resolvedParameterName, spec.min, spec.max, spec.step, spec.defaultValue, spec.label, isAutomatable));
    }

    layout.add(std::move(singleGroup));

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>(makeBandGroupId(bandIndex),
                                                                          makeBandGroupName(bandIndex),
                                                                          " | ");

        for (size_t parameterIndex = 0; parameterIndex < parameterSpecs.size(); ++parameterIndex)
        {
            const auto& spec = parameterSpecs[parameterIndex];
            const auto slot = static_cast<ParameterSlot>(parameterIndex);
            const auto parameterId = makeBandParameterId(bandIndex, spec.suffix);
            const auto isAutomatable = isAutomationOnlySlot(slot);
            const auto parameterName = isAutomatable ? makeBandAutomationParameterName(bandIndex, slot)
                                                     : juce::String(spec.name);

            if (spec.type == ParameterType::boolean)
                group->addChild(boolParam(parameterId, parameterName, spec.defaultValue >= 0.5f, isAutomatable));
            else
                group->addChild(floatParam(parameterId, parameterName, spec.min, spec.max, spec.step, spec.defaultValue, spec.label, isAutomatable));
        }

        layout.add(std::move(group));
    }

    return layout;
}
} // namespace

Mx6AudioProcessor::Mx6AudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    cacheParameterPointers();
}

Mx6AudioProcessor::~Mx6AudioProcessor() = default;

void Mx6AudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
{
    multibandProcessor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    singleBandProcessor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    syncParameters();
    multibandProcessor.reset();
    singleBandProcessor.reset();
    setLatencySamples(currentSingleBandMode ? singleBandProcessor.getLatencySamples()
                                            : multibandProcessor.getLatencySamples());
}

void Mx6AudioProcessor::releaseResources()
{
}

bool Mx6AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainOutput)
        return false;

    return mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
}

void Mx6AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    syncParameters();

    if (currentSingleBandMode)
        singleBandProcessor.process(buffer);
    else
        multibandProcessor.process(buffer);
}

juce::AudioProcessorEditor* Mx6AudioProcessor::createEditor()
{
    return new Mx6AudioProcessorEditor(*this);
}

bool Mx6AudioProcessor::hasEditor() const
{
    return true;
}

const juce::String Mx6AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Mx6AudioProcessor::acceptsMidi() const
{
    return false;
}

bool Mx6AudioProcessor::producesMidi() const
{
    return false;
}

bool Mx6AudioProcessor::isMidiEffect() const
{
    return false;
}

double Mx6AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Mx6AudioProcessor::getNumPrograms()
{
    return 1;
}

int Mx6AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Mx6AudioProcessor::setCurrentProgram(const int index)
{
    juce::ignoreUnused(index);
}

const juce::String Mx6AudioProcessor::getProgramName(const int index)
{
    juce::ignoreUnused(index);
    return {};
}

void Mx6AudioProcessor::changeProgramName(const int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void Mx6AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto stateXml = valueTreeState.copyState().createXml())
        copyXmlToBinary(*stateXml, destData);
}

void Mx6AudioProcessor::setStateInformation(const void* data, const int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorValueTreeState& Mx6AudioProcessor::getValueTreeState() noexcept
{
    return valueTreeState;
}

const juce::AudioProcessorValueTreeState& Mx6AudioProcessor::getValueTreeState() const noexcept
{
    return valueTreeState;
}

juce::AudioProcessorValueTreeState::ParameterLayout Mx6AudioProcessor::createParameterLayout()
{
    return buildParameterLayout();
}

void Mx6AudioProcessor::cacheParameterPointers()
{
    rawSingleBandModeParameter = valueTreeState.getRawParameterValue(makeSingleBandModeParameterId());
    jassert(rawSingleBandModeParameter != nullptr);

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        rawSoloParameters[bandIndex] = valueTreeState.getRawParameterValue(makeSoloParameterId(bandIndex));
        jassert(rawSoloParameters[bandIndex] != nullptr);
    }

    for (size_t parameterIndex = 0; parameterIndex < numFullbandAutomationSlots; ++parameterIndex)
    {
        rawFullbandParameters[parameterIndex] = valueTreeState.getRawParameterValue(makeFullbandParameterId(fullbandAutomationSpecs[parameterIndex].suffix));
        jassert(rawFullbandParameters[parameterIndex] != nullptr);
    }

    for (size_t parameterIndex = 0; parameterIndex < numFullbandVisibleSlots; ++parameterIndex)
    {
        rawFullbandVisibleParameters[parameterIndex] = valueTreeState.getRawParameterValue(makeFullbandParameterId(fullbandVisibleSpecs[parameterIndex].suffix));
        jassert(rawFullbandVisibleParameters[parameterIndex] != nullptr);
    }

    for (size_t parameterIndex = 0; parameterIndex < numParameterSlots; ++parameterIndex)
    {
        rawSingleParameters[parameterIndex] = valueTreeState.getRawParameterValue(makeSingleParameterId(parameterSpecs[parameterIndex].suffix));
        jassert(rawSingleParameters[parameterIndex] != nullptr);
    }

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        for (size_t parameterIndex = 0; parameterIndex < numParameterSlots; ++parameterIndex)
        {
            rawBandParameters[bandIndex][parameterIndex] = valueTreeState.getRawParameterValue(
                makeBandParameterId(bandIndex, parameterSpecs[parameterIndex].suffix));

            jassert(rawBandParameters[bandIndex][parameterIndex] != nullptr);
        }
    }
}

mx6::dsp::DspCore::Parameters Mx6AudioProcessor::readBandParameters(const size_t bandIndex) const
{
    const auto loadFloat = [this, bandIndex] (const ParameterSlot slot)
    {
        if (const auto* value = rawBandParameters[bandIndex][toIndex(slot)])
            return value->load();

        jassertfalse;
        return 0.0f;
    };

    const auto loadBool = [&loadFloat] (const ParameterSlot slot)
    {
        return loadFloat(slot) >= 0.5f;
    };

    mx6::dsp::DspCore::Parameters parameters;
    parameters.inGn = loadFloat(ParameterSlot::inGn);
    parameters.inRight = loadFloat(ParameterSlot::inRight);
    parameters.inLeft = loadFloat(ParameterSlot::inLeft);
    parameters.autoInGn = loadFloat(ParameterSlot::autoInGn);
    parameters.autoInRight = loadFloat(ParameterSlot::autoInRight);
    parameters.autoInLeft = loadFloat(ParameterSlot::autoInLeft);
    parameters.wide = loadFloat(ParameterSlot::wide);
    parameters.thLU = loadFloat(ParameterSlot::thLU);
    parameters.mkLU = loadFloat(ParameterSlot::mkLU);
    parameters.thLD = loadFloat(ParameterSlot::thLD);
    parameters.mkLD = loadFloat(ParameterSlot::mkLD);
    parameters.thRU = loadFloat(ParameterSlot::thRU);
    parameters.mkRU = loadFloat(ParameterSlot::mkRU);
    parameters.thRD = loadFloat(ParameterSlot::thRD);
    parameters.mkRD = loadFloat(ParameterSlot::mkRD);
    parameters.hwBypass = loadBool(ParameterSlot::hwBypass);
    parameters.LLThResh = loadFloat(ParameterSlot::LLThResh);
    parameters.LLTension = loadFloat(ParameterSlot::LLTension);
    parameters.LLRelease = loadFloat(ParameterSlot::LLRelease);
    parameters.LLmk = loadFloat(ParameterSlot::LLmk);
    parameters.RRThResh = loadFloat(ParameterSlot::RRThResh);
    parameters.RRTension = loadFloat(ParameterSlot::RRTension);
    parameters.RRRelease = loadFloat(ParameterSlot::RRRelease);
    parameters.RRmk = loadFloat(ParameterSlot::RRmk);
    parameters.DMbypass = loadBool(ParameterSlot::DMbypass);
    parameters.FFThResh = loadFloat(ParameterSlot::FFThResh);
    parameters.FFTension = loadFloat(ParameterSlot::FFTension);
    parameters.FFRelease = loadFloat(ParameterSlot::FFRelease);
    parameters.FFmk = loadFloat(ParameterSlot::FFmk);
    parameters.FFbypass = loadBool(ParameterSlot::FFbypass);
    parameters.moRph = loadFloat(ParameterSlot::moRph);
    parameters.peakHoldHz = loadFloat(ParameterSlot::peakHoldHz);
    parameters.TensionFlooR = loadFloat(ParameterSlot::TensionFlooR);
    parameters.TensionHysT = loadFloat(ParameterSlot::TensionHysT);
    parameters.delTa = loadBool(ParameterSlot::delTa);
    return parameters;
}

mx6::dsp::DspCore::Parameters Mx6AudioProcessor::readSingleParameters() const
{
    const auto loadFloat = [this] (const ParameterSlot slot)
    {
        if (const auto* value = rawSingleParameters[toIndex(slot)])
            return value->load();

        jassertfalse;
        return 0.0f;
    };

    const auto loadBool = [&loadFloat] (const ParameterSlot slot)
    {
        return loadFloat(slot) >= 0.5f;
    };

    mx6::dsp::DspCore::Parameters parameters;
    parameters.inGn = loadFloat(ParameterSlot::inGn);
    parameters.inRight = loadFloat(ParameterSlot::inRight);
    parameters.inLeft = loadFloat(ParameterSlot::inLeft);
    parameters.autoInGn = loadFloat(ParameterSlot::autoInGn);
    parameters.autoInRight = loadFloat(ParameterSlot::autoInRight);
    parameters.autoInLeft = loadFloat(ParameterSlot::autoInLeft);
    parameters.wide = loadFloat(ParameterSlot::wide);
    parameters.thLU = loadFloat(ParameterSlot::thLU);
    parameters.mkLU = loadFloat(ParameterSlot::mkLU);
    parameters.thLD = loadFloat(ParameterSlot::thLD);
    parameters.mkLD = loadFloat(ParameterSlot::mkLD);
    parameters.thRU = loadFloat(ParameterSlot::thRU);
    parameters.mkRU = loadFloat(ParameterSlot::mkRU);
    parameters.thRD = loadFloat(ParameterSlot::thRD);
    parameters.mkRD = loadFloat(ParameterSlot::mkRD);
    parameters.hwBypass = loadBool(ParameterSlot::hwBypass);
    parameters.LLThResh = loadFloat(ParameterSlot::LLThResh);
    parameters.LLTension = loadFloat(ParameterSlot::LLTension);
    parameters.LLRelease = loadFloat(ParameterSlot::LLRelease);
    parameters.LLmk = loadFloat(ParameterSlot::LLmk);
    parameters.RRThResh = loadFloat(ParameterSlot::RRThResh);
    parameters.RRTension = loadFloat(ParameterSlot::RRTension);
    parameters.RRRelease = loadFloat(ParameterSlot::RRRelease);
    parameters.RRmk = loadFloat(ParameterSlot::RRmk);
    parameters.DMbypass = loadBool(ParameterSlot::DMbypass);
    parameters.FFThResh = loadFloat(ParameterSlot::FFThResh);
    parameters.FFTension = loadFloat(ParameterSlot::FFTension);
    parameters.FFRelease = loadFloat(ParameterSlot::FFRelease);
    parameters.FFmk = loadFloat(ParameterSlot::FFmk);
    parameters.FFbypass = loadBool(ParameterSlot::FFbypass);
    parameters.moRph = loadFloat(ParameterSlot::moRph);
    parameters.peakHoldHz = loadFloat(ParameterSlot::peakHoldHz);
    parameters.TensionFlooR = loadFloat(ParameterSlot::TensionFlooR);
    parameters.TensionHysT = loadFloat(ParameterSlot::TensionHysT);
    parameters.delTa = loadBool(ParameterSlot::delTa);
    return parameters;
}

mx6::dsp::MultibandProcessor::FullbandParameters Mx6AudioProcessor::readFullbandParameters() const
{
    const auto loadAutomationFloat = [this] (const FullbandAutomationSlot slot)
    {
        if (const auto* value = rawFullbandParameters[static_cast<size_t>(slot)])
            return value->load();

        jassertfalse;
        return 0.0f;
    };

    const auto loadVisibleFloat = [this] (const FullbandVisibleSlot slot)
    {
        if (const auto* value = rawFullbandVisibleParameters[static_cast<size_t>(slot)])
            return value->load();

        jassertfalse;
        return 0.0f;
    };

    mx6::dsp::MultibandProcessor::FullbandParameters parameters;
    parameters.inGn = loadVisibleFloat(FullbandVisibleSlot::inGn);
    parameters.outGn = loadVisibleFloat(FullbandVisibleSlot::outGn);
    parameters.autoInGn = loadAutomationFloat(FullbandAutomationSlot::inGn);
    parameters.autoInRight = loadAutomationFloat(FullbandAutomationSlot::inRight);
    parameters.autoInLeft = loadAutomationFloat(FullbandAutomationSlot::inLeft);
    return parameters;
}

mx6::dsp::MultibandProcessor::SoloMask Mx6AudioProcessor::readSoloMask() const
{
    mx6::dsp::MultibandProcessor::SoloMask soloMask {};

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        if (const auto* value = rawSoloParameters[bandIndex])
            soloMask[bandIndex] = value->load() >= 0.5f;
        else
            jassertfalse;
    }

    return soloMask;
}

bool Mx6AudioProcessor::readSingleBandMode() const
{
    if (const auto* value = rawSingleBandModeParameter)
        return value->load() >= 0.5f;

    jassertfalse;
    return false;
}

void Mx6AudioProcessor::syncParameters()
{
    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
        currentBandParameters[bandIndex] = readBandParameters(bandIndex);

    currentFullbandParameters = readFullbandParameters();
    currentSingleParameters = readSingleParameters();
    currentSoloMask = readSoloMask();
    currentSingleBandMode = readSingleBandMode();
    multibandProcessor.setBandParameters(currentBandParameters);
    multibandProcessor.setFullbandParameters(currentFullbandParameters);
    multibandProcessor.setSoloMask(currentSoloMask);

    singleBandProcessor.setParameters(currentSingleParameters);
    setLatencySamples(currentSingleBandMode ? singleBandProcessor.getLatencySamples()
                                            : multibandProcessor.getLatencySamples());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Mx6AudioProcessor();
}
