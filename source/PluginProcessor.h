#pragma once

#include <JuceHeader.h>

#include "dsp/MultibandProcessor.h"

#include <array>
#include <atomic>

class Mx6AudioProcessor final : public juce::AudioProcessor
{
public:
    Mx6AudioProcessor();
    ~Mx6AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept;
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    static constexpr size_t numBands = mx6::dsp::MultibandProcessor::numBands;
    static constexpr size_t numParameterSlots = 35;
    static constexpr size_t numFullbandVisibleSlots = 2;
    static constexpr size_t numFullbandAutomationSlots = 3;

    void cacheParameterPointers();
    mx6::dsp::DspCore::Parameters readBandParameters(size_t bandIndex) const;
    mx6::dsp::DspCore::Parameters readSingleParameters() const;
    mx6::dsp::MultibandProcessor::FullbandParameters readFullbandParameters() const;
    mx6::dsp::MultibandProcessor::SoloMask readSoloMask() const;
    bool readSingleBandMode() const;
    void syncParameters();

    juce::AudioProcessorValueTreeState valueTreeState;
    mx6::dsp::MultibandProcessor multibandProcessor;
    mx6::dsp::DspCore singleBandProcessor;
    std::atomic<float>* rawSingleBandModeParameter = nullptr;
    std::array<std::atomic<float>*, numBands> rawSoloParameters {};
    std::array<std::atomic<float>*, numFullbandVisibleSlots> rawFullbandVisibleParameters {};
    std::array<std::atomic<float>*, numFullbandAutomationSlots> rawFullbandParameters {};
    std::array<std::atomic<float>*, numParameterSlots> rawSingleParameters {};
    std::array<std::array<std::atomic<float>*, numParameterSlots>, numBands> rawBandParameters {};
    mx6::dsp::MultibandProcessor::FullbandParameters currentFullbandParameters {};
    mx6::dsp::DspCore::Parameters currentSingleParameters {};
    std::array<mx6::dsp::DspCore::Parameters, numBands> currentBandParameters {};
    mx6::dsp::MultibandProcessor::SoloMask currentSoloMask {};
    bool currentSingleBandMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mx6AudioProcessor)
};
