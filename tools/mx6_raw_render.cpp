#include <JuceHeader.h>

#include "src/dsp/DspCore.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace
{
using Parameters = mx6::dsp::DspCore::Parameters;
using Setter = std::function<void(Parameters&, const juce::String&)>;

bool parseBool(const juce::String& text)
{
    const auto lowered = text.trim().toLowerCase();

    if (lowered == "1" || lowered == "true" || lowered == "on" || lowered == "yes")
        return true;

    if (lowered == "0" || lowered == "false" || lowered == "off" || lowered == "no")
        return false;

    throw std::runtime_error("invalid boolean value: " + text.toStdString());
}

double parseDouble(const juce::String& text)
{
    const auto trimmed = text.trim();

    if (! trimmed.containsOnly("0123456789+-.eE"))
        throw std::runtime_error("invalid numeric value: " + text.toStdString());

    return trimmed.getDoubleValue();
}

int parseInt(const juce::String& text)
{
    const auto trimmed = text.trim();

    if (! trimmed.containsOnly("0123456789+-"))
        throw std::runtime_error("invalid integer value: " + text.toStdString());

    return trimmed.getIntValue();
}

const std::unordered_map<std::string, Setter>& getSetters()
{
    static const std::unordered_map<std::string, Setter> setters {
        { "inGn", [] (Parameters& p, const juce::String& v) { p.inGn = static_cast<float>(parseDouble(v)); } },
        { "thLU", [] (Parameters& p, const juce::String& v) { p.thLU = static_cast<float>(parseDouble(v)); } },
        { "mkLU", [] (Parameters& p, const juce::String& v) { p.mkLU = static_cast<float>(parseDouble(v)); } },
        { "thLD", [] (Parameters& p, const juce::String& v) { p.thLD = static_cast<float>(parseDouble(v)); } },
        { "mkLD", [] (Parameters& p, const juce::String& v) { p.mkLD = static_cast<float>(parseDouble(v)); } },
        { "thRU", [] (Parameters& p, const juce::String& v) { p.thRU = static_cast<float>(parseDouble(v)); } },
        { "mkRU", [] (Parameters& p, const juce::String& v) { p.mkRU = static_cast<float>(parseDouble(v)); } },
        { "thRD", [] (Parameters& p, const juce::String& v) { p.thRD = static_cast<float>(parseDouble(v)); } },
        { "mkRD", [] (Parameters& p, const juce::String& v) { p.mkRD = static_cast<float>(parseDouble(v)); } },
        { "hwBypass", [] (Parameters& p, const juce::String& v) { p.hwBypass = parseBool(v); } },
        { "LLThResh", [] (Parameters& p, const juce::String& v) { p.LLThResh = static_cast<float>(parseDouble(v)); } },
        { "LLTension", [] (Parameters& p, const juce::String& v) { p.LLTension = static_cast<float>(parseDouble(v)); } },
        { "LLRelease", [] (Parameters& p, const juce::String& v) { p.LLRelease = static_cast<float>(parseDouble(v)); } },
        { "LLmk", [] (Parameters& p, const juce::String& v) { p.LLmk = static_cast<float>(parseDouble(v)); } },
        { "RRThResh", [] (Parameters& p, const juce::String& v) { p.RRThResh = static_cast<float>(parseDouble(v)); } },
        { "RRTension", [] (Parameters& p, const juce::String& v) { p.RRTension = static_cast<float>(parseDouble(v)); } },
        { "RRRelease", [] (Parameters& p, const juce::String& v) { p.RRRelease = static_cast<float>(parseDouble(v)); } },
        { "RRmk", [] (Parameters& p, const juce::String& v) { p.RRmk = static_cast<float>(parseDouble(v)); } },
        { "DMbypass", [] (Parameters& p, const juce::String& v) { p.DMbypass = parseBool(v); } },
        { "FFThResh", [] (Parameters& p, const juce::String& v) { p.FFThResh = static_cast<float>(parseDouble(v)); } },
        { "FFTension", [] (Parameters& p, const juce::String& v) { p.FFTension = static_cast<float>(parseDouble(v)); } },
        { "FFRelease", [] (Parameters& p, const juce::String& v) { p.FFRelease = static_cast<float>(parseDouble(v)); } },
        { "FFmk", [] (Parameters& p, const juce::String& v) { p.FFmk = static_cast<float>(parseDouble(v)); } },
        { "FFbypass", [] (Parameters& p, const juce::String& v) { p.FFbypass = parseBool(v); } },
        { "moRph", [] (Parameters& p, const juce::String& v) { p.moRph = static_cast<float>(parseDouble(v)); } },
        { "peakHoldHz", [] (Parameters& p, const juce::String& v) { p.peakHoldHz = static_cast<float>(parseDouble(v)); } },
        { "TensionFlooR", [] (Parameters& p, const juce::String& v) { p.TensionFlooR = static_cast<float>(parseDouble(v)); } },
        { "TensionHysT", [] (Parameters& p, const juce::String& v) { p.TensionHysT = static_cast<float>(parseDouble(v)); } },
        { "delTa", [] (Parameters& p, const juce::String& v) { p.delTa = parseBool(v); } },
    };

    return setters;
}

void applyParameter(Parameters& parameters, const juce::String& assignment)
{
    const auto separator = assignment.indexOfChar('=');

    if (separator <= 0 || separator == assignment.length() - 1)
        throw std::runtime_error("expected parameter assignment in the form id=value");

    const auto key = assignment.substring(0, separator).trim().toStdString();
    const auto value = assignment.substring(separator + 1).trim();

    const auto iterator = getSetters().find(key);

    if (iterator == getSetters().end())
        throw std::runtime_error("unknown parameter id: " + key);

    iterator->second(parameters, value);
}

void printUsage()
{
    std::cout
        << "usage: mx6_raw_render <input_wav> <output_wav> [--block-size N] [--param id=value ...]\n";
}

juce::AudioBuffer<float> loadAudioFile(const juce::File& inputFile, double& sampleRate)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(inputFile));

    if (reader == nullptr)
        throw std::runtime_error("failed to open input file: " + inputFile.getFullPathName().toStdString());

    sampleRate = reader->sampleRate;
    const auto numChannels = juce::jmax(1, static_cast<int>(reader->numChannels));
    const auto numSamples = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    if (! reader->read(&buffer, 0, numSamples, 0, true, true))
        throw std::runtime_error("failed to read input file: " + inputFile.getFullPathName().toStdString());

    return buffer;
}

void writeAudioFile(const juce::File& outputFile, const juce::AudioBuffer<float>& buffer, const double sampleRate)
{
    juce::WavAudioFormat wavFormat;
    auto outputStream = outputFile.createOutputStream();

    if (outputStream == nullptr)
        throw std::runtime_error("failed to open output file: " + outputFile.getFullPathName().toStdString());

    auto writer = std::unique_ptr<juce::AudioFormatWriter>(wavFormat.createWriterFor(outputStream.get(),
                                                                                     sampleRate,
                                                                                     static_cast<unsigned int>(buffer.getNumChannels()),
                                                                                     24,
                                                                                     {},
                                                                                     0));

    if (writer == nullptr)
        throw std::runtime_error("failed to create wav writer: " + outputFile.getFullPathName().toStdString());

    outputStream.release();

    if (! writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
        throw std::runtime_error("failed to write output file: " + outputFile.getFullPathName().toStdString());
}

int run(int argc, char* argv[])
{
    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    const auto inputFile = juce::File(argv[1]);
    const auto outputFile = juce::File(argv[2]);

    if (! inputFile.existsAsFile())
        throw std::runtime_error("input file not found: " + inputFile.getFullPathName().toStdString());

    auto blockSize = 512;
    Parameters parameters;

    for (int index = 3; index < argc; ++index)
    {
        const juce::String argument(argv[index]);

        if (argument == "--block-size")
        {
            if (++index >= argc)
                throw std::runtime_error("expected integer after --block-size");

            blockSize = parseInt(argv[index]);

            if (blockSize <= 0)
                throw std::runtime_error("block size must be positive");

            continue;
        }

        if (argument == "--param")
        {
            if (++index >= argc)
                throw std::runtime_error("expected id=value after --param");

            applyParameter(parameters, argv[index]);
            continue;
        }

        throw std::runtime_error("unknown argument: " + argument.toStdString());
    }

    auto sampleRate = 44100.0;
    auto inputBuffer = loadAudioFile(inputFile, sampleRate);
    juce::AudioBuffer<float> outputBuffer(inputBuffer.getNumChannels(), inputBuffer.getNumSamples());

    mx6::dsp::DspCore dspCore;
    dspCore.prepare(sampleRate, blockSize, inputBuffer.getNumChannels());
    dspCore.reset();
    dspCore.setParameters(parameters);

    juce::AudioBuffer<float> scratchBuffer(inputBuffer.getNumChannels(), blockSize);

    for (int offset = 0; offset < inputBuffer.getNumSamples(); offset += blockSize)
    {
        const auto numThisTime = juce::jmin(blockSize, inputBuffer.getNumSamples() - offset);
        scratchBuffer.clear();

        for (int channel = 0; channel < inputBuffer.getNumChannels(); ++channel)
            scratchBuffer.copyFrom(channel, 0, inputBuffer, channel, offset, numThisTime);

        juce::AudioBuffer<float> blockView(scratchBuffer.getArrayOfWritePointers(),
                                           scratchBuffer.getNumChannels(),
                                           numThisTime);
        dspCore.process(blockView);

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            outputBuffer.copyFrom(channel, offset, blockView, channel, 0, numThisTime);
    }

    outputFile.getParentDirectory().createDirectory();
    writeAudioFile(outputFile, outputBuffer, sampleRate);

    std::cout
        << "input=" << inputFile.getFullPathName() << "\n"
        << "output=" << outputFile.getFullPathName() << "\n"
        << "sample_rate=" << sampleRate << "\n"
        << "block_size=" << blockSize << "\n"
        << "latency_samples=" << dspCore.getLatencySamples() << "\n";

    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
    return juce::ConsoleApplication::invokeCatchingFailures([argc, argv]
    {
        return run(argc, argv);
    });
}
