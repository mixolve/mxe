#include "EditorPages.h"

#include "EditorUiState.h"

namespace mxe::editor
{
BandPageComponent::BandPageComponent(juce::AudioProcessorValueTreeState& state,
                                     const size_t bandIndexIn,
                                     const std::function<juce::String(const char*)>& parameterIdProvider,
                                     const juce::Colour accent,
                                     std::function<void()> onSoloClickIn,
                                     std::function<bool()> isSoloActiveIn,
                                     std::function<bool()> isSoloEnabledIn)
    : valueTreeState(&state),
      bandIndex(bandIndexIn),
      onSoloClick(std::move(onSoloClickIn)),
      isSoloActive(std::move(isSoloActiveIn)),
      isSoloEnabled(std::move(isSoloEnabledIn)),
      halfWave(state,
               parameterIdProvider,
               halfWaveSection,
               accent,
               [this] { refreshLayout(); },
               [this] (bool expanded) { handleSectionExpanded(0, expanded); },
               {}, {}, {}, {}, {}, {},
               valueTreeState,
               mxe::editor::uiState::makeBandSectionExpandedStateKey(bandIndex, 0)),
      dm(state,
         parameterIdProvider,
         dmSection,
         accent,
         [this] { refreshLayout(); },
         [this] (bool expanded) { handleSectionExpanded(1, expanded); },
         {}, {}, {}, {}, {}, {},
         valueTreeState,
         mxe::editor::uiState::makeBandSectionExpandedStateKey(bandIndex, 1)),
      ff(state,
         parameterIdProvider,
         ffSection,
         accent,
         [this] { refreshLayout(); },
         [this] (bool expanded) { handleSectionExpanded(2, expanded); },
         {}, {}, {}, {}, {}, {},
         valueTreeState,
         mxe::editor::uiState::makeBandSectionExpandedStateKey(bandIndex, 2)),
      global(state,
              parameterIdProvider,
              globalSection,
              accent,
              [this] { refreshLayout(); },
              [this] (bool expanded) { handleSectionExpanded(3, expanded); },
              {},
              [this] (const char* suffix)
              {
                  if (juce::String(suffix) == "delTa" && onSoloClick)
                      onSoloClick();
              },
              [this] (const char* suffix)
              {
                  return juce::String(suffix) == "delTa" && isSoloActive != nullptr && isSoloActive();
              },
              [this] (const char* suffix)
              {
                  return juce::String(suffix) == "delTa" && (isSoloEnabled == nullptr || isSoloEnabled());
              },
              {}, {},
              valueTreeState,
              mxe::editor::uiState::makeBandSectionExpandedStateKey(bandIndex, 3))
{
    addAndMakeVisible(halfWave);
    addAndMakeVisible(dm);
    addAndMakeVisible(ff);
    addAndMakeVisible(global);
}

void BandPageComponent::refreshExternalState()
{
    global.refreshExternalState();
}

int BandPageComponent::getPreferredHeight() const
{
    return 2
        + halfWave.getPreferredHeight()
        + sectionRowGap
        + dm.getPreferredHeight()
        + sectionRowGap
        + ff.getPreferredHeight()
        + sectionRowGap
        + global.getPreferredHeight();
}

void BandPageComponent::resized()
{
    auto bounds = getLocalBounds().reduced(0, 1);

    auto sectionBounds = bounds.removeFromTop(halfWave.getPreferredHeight());
    halfWave.setBounds(sectionBounds);
    bounds.removeFromTop(sectionRowGap);
    sectionBounds = bounds.removeFromTop(dm.getPreferredHeight());
    dm.setBounds(sectionBounds);
    bounds.removeFromTop(sectionRowGap);
    sectionBounds = bounds.removeFromTop(ff.getPreferredHeight());
    ff.setBounds(sectionBounds);
    bounds.removeFromTop(sectionRowGap);
    sectionBounds = bounds.removeFromTop(global.getPreferredHeight());
    global.setBounds(sectionBounds);
}

void BandPageComponent::handleSectionExpanded(const int sectionIndex, const bool expanded)
{
    if (! expanded)
        return;

    if (sectionIndex != 0)
        halfWave.setExpanded(false);

    if (sectionIndex != 1)
        dm.setExpanded(false);

    if (sectionIndex != 2)
        ff.setExpanded(false);

    if (sectionIndex != 3)
        global.setExpanded(false);

    resized();
}

void BandPageComponent::refreshLayout()
{
    const auto preferredHeight = getPreferredHeight();

    if (getHeight() != preferredHeight)
        setSize(getWidth(), preferredHeight);
    else
        resized();
}

FullbandPageComponent::FullbandPageComponent(juce::AudioProcessorValueTreeState& state,
                                             const std::function<juce::String(const char*)>& parameterIdProvider,
                                             const bool autoSoloEnabled,
                                             std::function<size_t()> activeSplitCountProviderIn,
                                             std::function<void(int)> onActiveSplitCountChangeIn,
                                             std::function<void(bool)> onAutoSoloChangedIn)
    : valueTreeState(&state),
      activeSplitCountProvider(std::move(activeSplitCountProviderIn)),
      fullband(state,
               parameterIdProvider,
               fullbandSection,
               uiAccent,
               [this] { refreshLayout(); },
               [this] (const bool expanded)
               {
                   if (expanded)
                       crossover.setExpanded(false);
               },
               {}, {}, {}, {}, {}, {},
               valueTreeState,
               mxe::editor::uiState::makeFullbandSectionExpandedStateKey(0)),
      crossover(state,
                parameterIdProvider,
                crossoverSection,
                uiAccent,
                [this] { refreshLayout(); },
                [this] (const bool expanded)
                {
                    if (expanded)
                        fullband.setExpanded(false);
                },
                [&state, parameterIdProvider] (const char* suffix)
                {
                    const auto suffixString = juce::String(suffix);
                    auto parameterIndex = static_cast<size_t>(0);
                    auto found = false;

                    for (size_t index = 0; index < crossoverControls.size(); ++index)
                    {
                        if (suffixString == crossoverControls[index].suffix)
                        {
                            parameterIndex = index;
                            found = true;
                            break;
                        }
                    }

                    if (! found)
                        return ValueConstraint {};

                    return ValueConstraint { [&state, parameterIdProvider, parameterIndex] (const float value)
                    {
                        auto lowerBound = 20.0f;
                        auto upperBound = 20000.0f;

                        if (parameterIndex > 0)
                        {
                            if (auto* previousParameter = dynamic_cast<juce::RangedAudioParameter*>(
                                    state.getParameter(parameterIdProvider(crossoverControls[parameterIndex - 1].suffix))))
                            {
                                lowerBound = previousParameter->convertFrom0to1(previousParameter->getValue()) + crossoverMinGapHz;
                            }
                        }

                        if (parameterIndex + 1 < crossoverControls.size())
                        {
                            if (auto* nextParameter = dynamic_cast<juce::RangedAudioParameter*>(
                                    state.getParameter(parameterIdProvider(crossoverControls[parameterIndex + 1].suffix))))
                            {
                                upperBound = nextParameter->convertFrom0to1(nextParameter->getValue()) - crossoverMinGapHz;
                            }
                        }

                        return juce::jlimit(lowerBound, juce::jmax(lowerBound, upperBound), value);
                    } };
                },
                {},
                {},
                {},
                []
                {
                    return static_cast<size_t>(1);
                },
                [this]
                {
                    return activeSplitCountProvider != nullptr
                        ? juce::jmin(crossoverControls.size(), activeSplitCountProvider())
                        : crossoverControls.size();
                },
                valueTreeState,
                mxe::editor::uiState::makeFullbandSectionExpandedStateKey(1)),
      autoSoloButton(uiAccent),
      addCrossoverButton(uiAccent),
      removeCrossoverButton(uiAccent),
      onActiveSplitCountChange(std::move(onActiveSplitCountChangeIn)),
      onAutoSoloChanged(std::move(onAutoSoloChangedIn))
{
    const auto restoredAutoSoloEnabled = mxe::editor::uiState::getBool(state.state,
                                                                      mxe::editor::uiState::autoSoloEnabledId(),
                                                                      autoSoloEnabled);

    addAndMakeVisible(fullband);
    addAndMakeVisible(crossover);

    autoSoloButton.setButtonText("AUTO-SOLO");
    autoSoloButton.setClickingTogglesState(true);
    autoSoloButton.setToggleState(restoredAutoSoloEnabled, juce::dontSendNotification);
    autoSoloButton.onClick = [this]
    {
        if (onAutoSoloChanged)
            onAutoSoloChanged(autoSoloButton.getToggleState());

        persistAutoSoloState();
    };
    addAndMakeVisible(autoSoloButton);

    addCrossoverButton.setButtonText("XOV-ADD");
    addCrossoverButton.setClickingTogglesState(false);
    addCrossoverButton.onClick = [this]
    {
        if (onActiveSplitCountChange)
            onActiveSplitCountChange(1);
    };
    crossover.addAndMakeVisible(addCrossoverButton);

    removeCrossoverButton.setButtonText("XOV-DEL");
    removeCrossoverButton.setClickingTogglesState(false);
    removeCrossoverButton.onClick = [this]
    {
        if (onActiveSplitCountChange)
            onActiveSplitCountChange(-1);
    };
    addAndMakeVisible(removeCrossoverButton);
}

void FullbandPageComponent::persistAutoSoloState()
{
    if (valueTreeState == nullptr)
        return;

    mxe::editor::uiState::setBool(valueTreeState->state,
                                  mxe::editor::uiState::autoSoloEnabledId(),
                                  autoSoloButton.getToggleState());
    mxe::editor::uiState::setBool(valueTreeState->state, mxe::editor::uiState::hasUiStateId(), true);
}

void FullbandPageComponent::refreshLayout()
{
    const auto preferredHeight = getPreferredHeight();

    if (getHeight() != preferredHeight)
        setSize(getWidth(), preferredHeight);
    else
        resized();
}

int FullbandPageComponent::getPreferredHeight() const
{
    auto height = 2
        + fullband.getPreferredHeight()
        + sectionRowGap
        + crossover.getPreferredHeight();

    if (crossover.isExpanded())
        height += sectionRowGap + buttonHeight;

    height += sectionRowGap + buttonHeight;

    return height;
}

void FullbandPageComponent::refreshExternalState()
{
    crossover.refreshExternalState();
}

void FullbandPageComponent::resized()
{
    auto bounds = getLocalBounds().reduced(0, 1);
    fullband.setBounds(bounds.removeFromTop(fullband.getPreferredHeight()));
    bounds.removeFromTop(sectionRowGap);
    crossover.setBounds(bounds.removeFromTop(crossover.getPreferredHeight()));
    const auto addButtonBounds = crossover.getExtraControlBounds(0, {});
    addCrossoverButton.setVisible(! addButtonBounds.isEmpty());

    if (! addButtonBounds.isEmpty())
        addCrossoverButton.setBounds(addButtonBounds);

    const auto showRemoveButton = crossover.isExpanded();
    removeCrossoverButton.setVisible(showRemoveButton);

    if (showRemoveButton)
    {
        bounds.removeFromTop(sectionRowGap);
        removeCrossoverButton.setBounds(bounds.removeFromTop(buttonHeight).reduced(sectionContentInsetX, 0));
    }
    else
    {
        removeCrossoverButton.setBounds({});
    }

    bounds.removeFromTop(sectionRowGap);
    autoSoloButton.setBounds(bounds.removeFromTop(buttonHeight).reduced(sectionContentInsetX, 0));
}
} // namespace mxe::editor
