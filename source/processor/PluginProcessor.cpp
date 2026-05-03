#include "PluginProcessor.h"

#include "EditorUiState.h"
#include "PluginEditor.h"

MxeAudioProcessor::MxeAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, &undoManager, "PARAMETERS", createParameterLayout())
{
    cacheParameterPointers();
}

MxeAudioProcessor::~MxeAudioProcessor() = default;

void MxeAudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
{
    multibandProcessor.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    syncParameters();
    multibandProcessor.reset();
    setLatencySamples(multibandProcessor.getLatencySamples());
}

void MxeAudioProcessor::releaseResources()
{
}

bool MxeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainOutput)
        return false;

    return mainOutput == juce::AudioChannelSet::mono()
        || mainOutput == juce::AudioChannelSet::stereo();
}

void MxeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    syncParameters();

    multibandProcessor.process(buffer);
}

juce::AudioProcessorEditor* MxeAudioProcessor::createEditor()
{
    return new MxeAudioProcessorEditor(*this);
}

bool MxeAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MxeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MxeAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MxeAudioProcessor::producesMidi() const
{
    return false;
}

bool MxeAudioProcessor::isMidiEffect() const
{
    return false;
}

double MxeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MxeAudioProcessor::getNumPrograms()
{
    return 1;
}

int MxeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MxeAudioProcessor::setCurrentProgram(const int index)
{
    juce::ignoreUnused(index);
}

const juce::String MxeAudioProcessor::getProgramName(const int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MxeAudioProcessor::changeProgramName(const int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void MxeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    mxe::editor::uiState::setInt(valueTreeState.state, mxe::editor::uiState::editorWidthId(), lastEditorWidth.load());
    mxe::editor::uiState::setInt(valueTreeState.state, mxe::editor::uiState::editorHeightId(), lastEditorHeight.load());

    if (auto stateXml = valueTreeState.copyState().createXml())
        copyXmlToBinary(*stateXml, destData);
}

void MxeAudioProcessor::setStateInformation(const void* data, const int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
    {
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
            setLastEditorSize(mxe::editor::uiState::getInt(valueTreeState.state,
                                                           mxe::editor::uiState::editorWidthId(),
                                                           defaultEditorWidth),
                              mxe::editor::uiState::getInt(valueTreeState.state,
                                                           mxe::editor::uiState::editorHeightId(),
                                                           defaultEditorHeight));
        }
    }
}

juce::AudioProcessorValueTreeState& MxeAudioProcessor::getValueTreeState() noexcept
{
    return valueTreeState;
}

const juce::AudioProcessorValueTreeState& MxeAudioProcessor::getValueTreeState() const noexcept
{
    return valueTreeState;
}

juce::UndoManager& MxeAudioProcessor::getUndoManager() noexcept
{
    return undoManager;
}

const juce::UndoManager& MxeAudioProcessor::getUndoManager() const noexcept
{
    return undoManager;
}

juce::Point<int> MxeAudioProcessor::getLastEditorSize() const noexcept
{
    return { lastEditorWidth.load(), lastEditorHeight.load() };
}

void MxeAudioProcessor::setLastEditorSize(const int width, const int height) noexcept
{
    lastEditorWidth.store(juce::jmax(1, width));
    lastEditorHeight.store(juce::jmax(1, height));
}

juce::AudioProcessorValueTreeState::ParameterLayout MxeAudioProcessor::createParameterLayout()
{
    return mxe::parameters::createParameterLayout();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MxeAudioProcessor();
}
