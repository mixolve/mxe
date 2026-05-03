#pragma once

#include <JuceHeader.h>

#include <functional>
#include <memory>

namespace mxe::editor
{
std::unique_ptr<juce::Component> makeTextPromptOverlay(juce::String currentText,
                                                       std::function<bool(const juce::String&)> onCommit,
                                                       std::function<void()> onDismiss,
                                                       std::function<void()> onClose,
                                                       std::function<juce::Rectangle<int>()> anchorBoundsProvider = {});

std::unique_ptr<juce::Component> makeInfoPromptOverlay(juce::String markdownText,
                                                       std::function<juce::Rectangle<int>()> anchorBoundsProvider,
                                                       std::function<void()> onClose);
} // namespace mxe::editor
