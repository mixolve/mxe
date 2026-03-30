#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

#include <array>
#include <memory>

class Mx6AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit Mx6AudioProcessorEditor(Mx6AudioProcessor&);
    ~Mx6AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr size_t numBands = mx6::dsp::MultibandProcessor::numBands;
    static constexpr size_t numMonitorButtons = numBands + 2;

    void selectBand(size_t bandIndex);
    void setAllBandsMonitoring();
    void toggleProcessingMode();
    void syncMonitorParameters();
    void updateMonitorButtons();
    void updateBandPageVisibility();
    void updatePageChangedIndicators();

    std::array<std::unique_ptr<juce::Button>, numMonitorButtons> monitorButtons;
    std::array<juce::RangedAudioParameter*, numBands> soloParameters {};
    juce::RangedAudioParameter* singleBandModeParameter = nullptr;
    std::array<std::unique_ptr<juce::Component>, numBands> bandPages;
    std::unique_ptr<juce::Component> allBandsPage;
    std::unique_ptr<juce::Component> singleBandPage;
    juce::Label footerLabel;
    size_t visibleBandIndex = 0;
    bool allBandsActive = true;
    bool singleBandMode = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mx6AudioProcessorEditor)
};
