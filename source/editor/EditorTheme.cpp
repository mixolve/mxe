#include "EditorTheme.h"

#include "ValueFormatting.h"

#include <cmath>

namespace mxe::editor
{
int getEditorInsetX(const int width) noexcept
{
    return juce::roundToInt(static_cast<float>(juce::jmax(0, width)) * editorInsetSideRatio);
}

int getEditorInsetTop(const int height) noexcept
{
    return juce::roundToInt(static_cast<float>(juce::jmax(0, height)) * editorInsetTopRatio);
}

int getEditorInsetBottom(const int height) noexcept
{
    return juce::roundToInt(static_cast<float>(juce::jmax(0, height)) * editorInsetBottomRatio);
}

int getScaledParameterNameWidth(const int rowWidth) noexcept
{
    const auto usableWidth = juce::jmax(0, rowWidth - parameterGap);
    return usableWidth / 2;
}

int getScaledParameterValueWidth(const int rowWidth) noexcept
{
    const auto usableWidth = juce::jmax(0, rowWidth - parameterGap);
    return usableWidth - getScaledParameterNameWidth(rowWidth);
}

juce::FontOptions makeUiFont(const int styleFlags, const float height)
{
#if JUCE_TARGET_HAS_BINARY_DATA
    const auto useBold = (styleFlags & juce::Font::bold) != 0;

    static const auto regularTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraCodeRegular_ttf,
                                                                                BinaryData::FiraCodeRegular_ttfSize);
    static const auto boldTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraCodeBold_ttf,
                                                                             BinaryData::FiraCodeBold_ttfSize);

    if (auto typeface = useBold ? boldTypeface : regularTypeface)
        return juce::FontOptions(typeface).withHeight(height);
#endif

    return juce::FontOptions("Fira Code", height, styleFlags);
}

juce::String formatValueBoxText(const double value)
{
    return mxe::formatting::formatDspValue(value);
}

juce::String makeBandName(const size_t bandIndex)
{
    return juce::String(static_cast<int>(bandIndex + 1));
}

juce::Colour bandColour(const size_t bandIndex)
{
    juce::ignoreUnused(bandIndex);
    return uiAccent;
}
} // namespace mxe::editor
