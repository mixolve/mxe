#include <JuceHeader.h>

#include "src/dsp/MultibandCrossover.h"
#include "src/dsp/MultibandProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 256;
constexpr int totalSamples = 32768;
constexpr int numChannels = 2;
constexpr double toneFrequency = 997.0;
constexpr double tolerancePeak = 1.0e-6;
constexpr double toleranceRms = 1.0e-7;

double makeInputSample(const int channel, const int sampleIndex)
{
    const auto t = static_cast<double>(sampleIndex) / sampleRate;
    const auto tone = 0.25 * std::sin(juce::MathConstants<double>::twoPi * toneFrequency * t * (channel == 0 ? 1.0 : 1.13));
    const auto impulse = sampleIndex % 4096 == (channel == 0 ? 0 : 31) ? 0.5 : 0.0;
    const auto chirp = 0.08 * std::sin(juce::MathConstants<double>::twoPi * t * t * 700.0);
    return tone + impulse + chirp;
}
} // namespace

int main()
{
    mx6::dsp::MultibandCrossover crossover;
    crossover.prepare(sampleRate);

    mx6::dsp::MultibandProcessor processor;
    processor.prepare(sampleRate, blockSize, numChannels);

    mx6::dsp::MultibandProcessor::BandParameters parameters {};
    processor.setBandParameters(parameters);
    processor.reset();

    const auto latencySamples = processor.getLatencySamples();

    juce::AudioBuffer<float> inputBuffer(numChannels, totalSamples);
    juce::AudioBuffer<float> crossoverBuffer(numChannels, totalSamples);
    juce::AudioBuffer<float> outputBuffer(numChannels, totalSamples);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* input = inputBuffer.getWritePointer(channel);

        for (int sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex)
            input[sampleIndex] = static_cast<float>(makeInputSample(channel, sampleIndex));
    }

    for (int sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex)
    {
        const auto bands = crossover.processSample(inputBuffer.getSample(0, sampleIndex),
                                                   inputBuffer.getSample(1, sampleIndex));

        auto sumLeft = 0.0;
        auto sumRight = 0.0;

        for (const auto& band : bands)
        {
            sumLeft += band.left;
            sumRight += band.right;
        }

        crossoverBuffer.setSample(0, sampleIndex, static_cast<float>(sumLeft));
        crossoverBuffer.setSample(1, sampleIndex, static_cast<float>(sumRight));
    }

    for (int startSample = 0; startSample < totalSamples; startSample += blockSize)
    {
        const auto samplesThisBlock = std::min(blockSize, totalSamples - startSample);
        juce::AudioBuffer<float> block(numChannels, samplesThisBlock);

        for (int channel = 0; channel < numChannels; ++channel)
            block.copyFrom(channel, 0, inputBuffer, channel, startSample, samplesThisBlock);

        juce::MidiBuffer midi;
        processor.process(block);

        for (int channel = 0; channel < numChannels; ++channel)
            outputBuffer.copyFrom(channel, startSample, block, channel, 0, samplesThisBlock);
    }

    auto crossoverPeakError = 0.0;
    auto crossoverSumSquares = 0.0;
    auto crossoverComparedSamples = 0;
    auto peakError = 0.0;
    auto sumSquares = 0.0;
    auto comparedSamples = 0;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* input = inputBuffer.getReadPointer(channel);
        const auto* crossoverOutput = crossoverBuffer.getReadPointer(channel);
        const auto* output = outputBuffer.getReadPointer(channel);

        for (int sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex)
        {
            const auto crossoverDiff = static_cast<double>(crossoverOutput[sampleIndex]) - static_cast<double>(input[sampleIndex]);
            crossoverPeakError = std::max(crossoverPeakError, std::abs(crossoverDiff));
            crossoverSumSquares += crossoverDiff * crossoverDiff;
            ++crossoverComparedSamples;
        }

        for (int sampleIndex = latencySamples; sampleIndex < totalSamples; ++sampleIndex)
        {
            const auto diff = static_cast<double>(output[sampleIndex]) - static_cast<double>(crossoverOutput[sampleIndex - latencySamples]);
            peakError = std::max(peakError, std::abs(diff));
            sumSquares += diff * diff;
            ++comparedSamples;
        }
    }

    const auto crossoverRmsError = crossoverComparedSamples > 0
        ? std::sqrt(crossoverSumSquares / static_cast<double>(crossoverComparedSamples))
        : 0.0;
    const auto rmsError = comparedSamples > 0 ? std::sqrt(sumSquares / static_cast<double>(comparedSamples)) : 0.0;

    std::cout << "crossover_peak_error=" << crossoverPeakError << '\n';
    std::cout << "crossover_rms_error=" << crossoverRmsError << '\n';
    std::cout << "latency_samples=" << latencySamples << '\n';
    std::cout << "processor_vs_crossover_peak_error=" << peakError << '\n';
    std::cout << "processor_vs_crossover_rms_error=" << rmsError << '\n';

    if (peakError > tolerancePeak || rmsError > toleranceRms)
    {
        std::cerr << "Multiband sanity check failed.\n";
        return 1;
    }

    std::cout << "Multiband sanity check passed.\n";
    return 0;
}
