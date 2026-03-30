#include "PluginEditor.h"

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace
{
constexpr int fixedEditorWidth = 294;
constexpr int fixedEditorHeight = 516;
constexpr int editorInsetX = 4;
constexpr int editorInsetBottom = 4;
constexpr int parameterGap = 2;
constexpr int footerHeight = 20;
constexpr int pageColumnWidth = fixedEditorWidth - (editorInsetX * 2);
constexpr int sectionWidth = pageColumnWidth;
constexpr int controlRowWidth = sectionWidth - 8;
constexpr int parameterValueWidth = 94;
constexpr int parameterNameWidth = controlRowWidth - parameterGap - parameterValueWidth;
constexpr int monitorButtonGap = 2;
constexpr float uiFontSize = 20.0f;
constexpr float valueBoxDragNormalisedPerPixel = 0.0080f;
constexpr float smoothWheelStepThreshold = 0.045f;

const auto uiWhite = juce::Colour(0xffffffff);
const auto uiBlack = juce::Colour(0xff000000);
const auto uiAccent = juce::Colour(0xff9999ff);
const auto uiChanged = juce::Colour(0xffffcc99);
const auto uiGrey950 = juce::Colour(0xff121212);
const auto uiGrey900 = juce::Colour(0xff1a1a1a);
const auto uiGrey800 = juce::Colour(0xff242424);
const auto uiGrey700 = juce::Colour(0xff363636);
const auto uiGrey500 = juce::Colour(0xff707070);

juce::FontOptions makeUiFont(const int styleFlags = juce::Font::plain, const float height = uiFontSize)
{
#if JUCE_TARGET_HAS_BINARY_DATA
    const auto useBold = (styleFlags & juce::Font::bold) != 0;

    static const auto regularTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::SometypeMonoRegular_ttf,
                                                                                BinaryData::SometypeMonoRegular_ttfSize);
    static const auto boldTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::SometypeMonoBold_ttf,
                                                                             BinaryData::SometypeMonoBold_ttfSize);

    if (auto typeface = useBold ? boldTypeface : regularTypeface)
        return juce::FontOptions(typeface).withHeight(height);
#endif

    return juce::FontOptions("Sometype Mono", height, styleFlags);
}

juce::String formatValueBoxText(const double value)
{
    auto rounded = std::round(value * 10.0) / 10.0;

    if (std::abs(rounded) < 0.05)
        rounded = 0.0;

    return juce::String::formatted("%+08.1f", rounded);
}

struct ControlSpec
{
    const char* suffix = "";
    const char* label = "";
    bool isToggle = false;
    bool tracksChangedState = true;
};

struct SectionSpec
{
    const char* title = "";
    std::span<const ControlSpec> controls;
    bool startsExpanded = false;
    bool staysExpandedOnSelfClick = false;
};

constexpr auto halfWaveControls = std::to_array<ControlSpec>({
    { "thLU", "L-UP-THRESHOLD" },
    { "mkLU", "L-UP-OUT-GAIN" },
    { "thLD", "L-DOWN-THRESHOLD" },
    { "mkLD", "L-DOWN-OUT-GAIN" },
    { "thRU", "R-UP-THRESHOLD" },
    { "mkRU", "R-UP-OUT-GAIN" },
    { "thRD", "R-DOWN-THRESHOLD" },
    { "mkRD", "R-DOWN-OUT-GAIN" },
    { "hwBypass", "BYPASS", true, false },
});

constexpr auto dmControls = std::to_array<ControlSpec>({
    { "LLThResh", "L-THRESHOLD" },
    { "LLTension", "L-TENSION" },
    { "LLRelease", "L-RELEASE" },
    { "LLmk", "L-OUT-GAIN" },
    { "RRThResh", "R-THRESHOLD" },
    { "RRTension", "R-TENSION" },
    { "RRRelease", "R-RELEASE" },
    { "RRmk", "R-OUT-GAIN" },
    { "DMbypass", "BYPASS", true, false },
});

constexpr auto ffControls = std::to_array<ControlSpec>({
    { "FFThResh", "THRESHOLD" },
    { "FFTension", "TENSION" },
    { "FFRelease", "RELEASE" },
    { "FFmk", "OUT-GAIN" },
    { "FFbypass", "BYPASS", true, false },
});

constexpr auto globalControls = std::to_array<ControlSpec>({
    { "inGn", "INPUT-GAIN" },
    { "inRight", "IN-RIGHT" },
    { "inLeft", "IN-LEFT" },
    { "wide", "WIDE" },
    { "moRph", "MORPH" },
    { "peakHoldHz", "PEAK-HOLD" },
    { "TensionFlooR", "TEN-FLOOR" },
    { "TensionHysT", "TEN-HYST" },
    { "delTa", "DELTA", true, false },
});

constexpr auto fullbandControls = std::to_array<ControlSpec>({
    { "inGnVisible", "IN-GAIN" },
    { "outGnVisible", "OUT-GAIN" },
});

const auto halfWaveSection = SectionSpec { "HALF-WAVE", halfWaveControls, false, false };
const auto dmSection = SectionSpec { "DUAL-MONO", dmControls, false, false };
const auto ffSection = SectionSpec { "STEREO", ffControls, false, false };
const auto globalSection = SectionSpec { "GLOBAL", globalControls, true, false };
const auto fullbandSection = SectionSpec { "FULLBAND", fullbandControls, true, false };

juce::String makeBandParameterId(const size_t bandIndex, const char* suffix)
{
    return "band" + juce::String(static_cast<int>(bandIndex + 1)) + "_" + suffix;
}

juce::String makeSingleParameterId(const char* suffix)
{
    return "single_" + juce::String(suffix);
}

juce::String makeFullbandParameterId(const char* suffix)
{
    return "fullband_" + juce::String(suffix);
}

juce::String makeSoloParameterId(const size_t bandIndex)
{
    return "soloBand" + juce::String(static_cast<int>(bandIndex + 1));
}

juce::String makeSingleBandModeParameterId()
{
    return "singleBandMode";
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

class WheelForwardingTextEditor final : public juce::TextEditor
{
public:
    explicit WheelForwardingTextEditor(juce::Slider& sliderToControl)
        : juce::TextEditor(sliderToControl.getName()),
          slider(sliderToControl)
    {
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        slider.mouseWheelMove(event.getEventRelativeTo(&slider), wheel);
    }

private:
    juce::Slider& slider;
};

class WheelForwardingSliderLabel final : public juce::Label
{
public:
    explicit WheelForwardingSliderLabel(juce::Slider& sliderToControl)
        : juce::Label({}, {}),
          slider(sliderToControl)
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        setEditable(false, false, false);
    }

    std::function<void()> onResetRequest;
    std::function<void(const juce::MouseEvent&)> onDragStart;
    std::function<void(const juce::MouseEvent&)> onDragMove;
    std::function<void(const juce::MouseEvent&)> onDragEnd;

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        slider.mouseWheelMove(event.getEventRelativeTo(&slider), wheel);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu())
            return;

        if (event.mods.isShiftDown())
            return;

        if (event.mods.isLeftButtonDown() && onDragStart)
            onDragStart(event);
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu() || event.mods.isShiftDown())
            return;

        if (event.mods.isLeftButtonDown() && onDragMove)
            onDragMove(event);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu())
        {
            showEditor();

            return;
        }

        if (event.mods.isShiftDown())
        {
            if (onResetRequest)
                onResetRequest();

            return;
        }

        if (onDragEnd)
            onDragEnd(event);
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        juce::ignoreUnused(event);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler(*this);
    }

protected:
    juce::TextEditor* createEditorComponent() override
    {
        auto* editor = new WheelForwardingTextEditor(slider);
        editor->applyFontToAllText(getLookAndFeel().getLabelFont(*this));
        editor->setJustification(juce::Justification::centredRight);
        copyAllExplicitColoursTo(*editor);
        return editor;
    }

private:
    juce::Slider& slider;
};

class ResettableValueSlider final : public juce::Slider
{
public:
    std::function<void()> onResetRequest;
    std::function<void(const juce::MouseEvent&)> onValueBoxDragStart;
    std::function<void(const juce::MouseEvent&)> onValueBoxDragMove;
    std::function<void(const juce::MouseEvent&)> onValueBoxDragEnd;
};

class ResettableParameterLabel final : public juce::Label
{
public:
    using juce::Label::Label;
};

class ValueBoxComponent final : public juce::Component
{
public:
    ValueBoxComponent(ResettableValueSlider& sliderToControl, juce::RangedAudioParameter* parameterToControl)
        : slider(sliderToControl),
          parameter(parameterToControl)
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    ~ValueBoxComponent() override
    {
        stopGlobalEditTracking();
    }

    std::function<void()> onResetRequest;

    void setOutlineColour(const juce::Colour colour)
    {
        if (outlineColour == colour)
            return;

        outlineColour = colour;

        if (editor != nullptr)
            editor->setColour(juce::TextEditor::outlineColourId, outlineColour);

        repaint();
    }

    void setHighlightColour(const juce::Colour colour)
    {
        if (highlightColour == colour)
            return;

        highlightColour = colour;

        if (editor != nullptr)
            editor->setColour(juce::TextEditor::highlightColourId, highlightColour);
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(uiGrey800);
        graphics.fillRect(getLocalBounds());

        graphics.setColour(outlineColour);
        graphics.drawRect(getLocalBounds(), 1);

        graphics.setColour(uiWhite);
        graphics.setFont(makeUiFont());
        graphics.drawFittedText(formatValueBoxText(slider.getValue()),
                                getLocalBounds().reduced(4, 0),
                                juce::Justification::centredRight,
                                1,
                                1.0f);
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds(getLocalBounds());
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        juce::ignoreUnused(event);

        if (parameter == nullptr)
            return;

        if (editor != nullptr)
            hideEditor(false);

        if (wheel.isInertial)
            return;

        const auto directionalDelta = (std::abs(wheel.deltaX) > std::abs(wheel.deltaY) ? -wheel.deltaX : wheel.deltaY)
            * (wheel.isReversed ? -1.0f : 1.0f);

        if (std::abs(directionalDelta) < 1.0e-6f)
            return;

        const auto range = parameter->getNormalisableRange();
        const auto interval = range.interval > 0.0f ? range.interval : juce::jmax(0.001f, (range.end - range.start) * 0.001f);

        int stepCount = 0;

        if (wheel.isSmooth)
        {
            smoothWheelAccumulator += directionalDelta;

            while (smoothWheelAccumulator >= smoothWheelStepThreshold)
            {
                ++stepCount;
                smoothWheelAccumulator -= smoothWheelStepThreshold;
            }

            while (smoothWheelAccumulator <= -smoothWheelStepThreshold)
            {
                --stepCount;
                smoothWheelAccumulator += smoothWheelStepThreshold;
            }
        }
        else
        {
            smoothWheelAccumulator = 0.0f;
            stepCount = directionalDelta > 0.0f ? 1 : -1;
        }

        if (stepCount == 0)
            return;

        const auto currentValue = parameter->convertFrom0to1(parameter->getValue());
        const auto unclampedValue = currentValue + (interval * static_cast<float>(stepCount));
        const auto legalValue = range.snapToLegalValue(juce::jlimit(range.start, range.end, unclampedValue));

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(legalValue));
        parameter->endChangeGesture();
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        auto* clickedComponent = event.originalComponent;
        const auto clickIsInsideThisValueBox = clickedComponent != nullptr
            && (clickedComponent == this || isParentOf(clickedComponent));

        if (editor != nullptr && ! clickIsInsideThisValueBox)
        {
            hideEditor(false);
            return;
        }

        if (! clickIsInsideThisValueBox)
            return;

        if (event.mods.isPopupMenu())
            return;

        if (event.mods.isShiftDown())
        {
            if (onResetRequest)
                onResetRequest();

            return;
        }

        if (! event.mods.isLeftButtonDown() || parameter == nullptr)
            return;

        dragStartNormalisedValue = parameter->getValue();

        if (! isDragging)
        {
            parameter->beginChangeGesture();
            isDragging = true;
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (! isDragging || parameter == nullptr)
            return;

        const auto deltaPixels = -event.getDistanceFromDragStartY();
        const auto newNormalisedValue = juce::jlimit(0.0f,
                                                     1.0f,
                                                     dragStartNormalisedValue + (static_cast<float>(deltaPixels) * valueBoxDragNormalisedPerPixel));

        parameter->setValueNotifyingHost(newNormalisedValue);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        if (isDragging && parameter != nullptr)
        {
            parameter->endChangeGesture();
            isDragging = false;
            return;
        }

        if (event.mods.isPopupMenu())
            showEditor();
    }

private:
    void showEditor()
    {
        if (editor != nullptr)
            return;

        auto textEditor = std::make_unique<WheelForwardingTextEditor>(slider);
        textEditor->setFont(makeUiFont());
        textEditor->setJustification(juce::Justification::centredRight);
        textEditor->setColour(juce::TextEditor::textColourId, uiWhite);
        textEditor->setColour(juce::TextEditor::backgroundColourId, uiGrey800);
        textEditor->setColour(juce::TextEditor::outlineColourId, outlineColour);
        textEditor->setColour(juce::TextEditor::focusedOutlineColourId, outlineColour);
        textEditor->setColour(juce::TextEditor::highlightColourId, highlightColour);
        textEditor->setText(formatValueBoxText(slider.getValue()), false);
        textEditor->onReturnKey = [this] { hideEditor(false); };
        textEditor->onEscapeKey = [this] { hideEditor(true); };
        textEditor->onFocusLost = [this] { hideEditor(false); };

        addAndMakeVisible(*textEditor);
        editor = std::move(textEditor);
        startGlobalEditTracking();
        resized();
        editor->grabKeyboardFocus();
        editor->selectAll();
    }

    void hideEditor(const bool discard)
    {
        if (editor == nullptr)
            return;

        if (! discard && parameter != nullptr)
        {
            const auto enteredValue = editor->getText().trim().getDoubleValue();
            const auto range = parameter->getNormalisableRange();
            const auto clampedValue = juce::jlimit(static_cast<double>(range.start),
                                                   static_cast<double>(range.end),
                                                   enteredValue);
            const auto legalValue = range.snapToLegalValue(static_cast<float>(clampedValue));

            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(legalValue));
            parameter->endChangeGesture();
        }

        removeChildComponent(editor.get());
        editor.reset();
        stopGlobalEditTracking();
        repaint();
    }

    void startGlobalEditTracking()
    {
        if (isTrackingGlobalClicks)
            return;

        juce::Desktop::getInstance().addGlobalMouseListener(this);
        isTrackingGlobalClicks = true;
    }

    void stopGlobalEditTracking()
    {
        if (! isTrackingGlobalClicks)
            return;

        juce::Desktop::getInstance().removeGlobalMouseListener(this);
        isTrackingGlobalClicks = false;
    }

    ResettableValueSlider& slider;
    juce::RangedAudioParameter* parameter = nullptr;
    juce::Colour outlineColour = uiGrey500;
    juce::Colour highlightColour = uiAccent;
    bool isDragging = false;
    bool isTrackingGlobalClicks = false;
    float dragStartNormalisedValue = 0.0f;
    float smoothWheelAccumulator = 0.0f;
    std::unique_ptr<WheelForwardingTextEditor> editor;
};

class Mx6LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    juce::Font getLabelFont(juce::Label&) override
    {
        return makeUiFont();
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = new WheelForwardingSliderLabel(slider);

        if (auto* resettableSlider = dynamic_cast<ResettableValueSlider*>(&slider))
        {
            label->onResetRequest = resettableSlider->onResetRequest;
            label->onDragStart = resettableSlider->onValueBoxDragStart;
            label->onDragMove = resettableSlider->onValueBoxDragMove;
            label->onDragEnd = resettableSlider->onValueBoxDragEnd;
        }

        label->setJustificationType(juce::Justification::centredRight);
        label->setKeyboardType(juce::TextInputTarget::decimalKeyboard);

        label->setColour(juce::Label::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
        label->setColour(juce::Label::backgroundColourId,
                         (slider.getSliderStyle() == juce::Slider::LinearBar || slider.getSliderStyle() == juce::Slider::LinearBarVertical)
                             ? juce::Colours::transparentBlack
                             : slider.findColour(juce::Slider::textBoxBackgroundColourId));
        label->setColour(juce::Label::outlineColourId, slider.findColour(juce::Slider::textBoxOutlineColourId));
        label->setColour(juce::TextEditor::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
        label->setColour(juce::TextEditor::backgroundColourId, slider.findColour(juce::Slider::textBoxBackgroundColourId));
        label->setColour(juce::TextEditor::outlineColourId, slider.findColour(juce::Slider::textBoxOutlineColourId));
        label->setColour(juce::TextEditor::focusedOutlineColourId, slider.findColour(juce::Slider::textBoxOutlineColourId));
        label->setColour(juce::TextEditor::highlightColourId, slider.findColour(juce::Slider::textBoxHighlightColourId));

        return label;
    }

    Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        return { {}, slider.getLocalBounds() };
    }

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override
    {
        juce::ignoreUnused(button);
        return juce::jlimit(14, 18, tabDepth);
    }

    juce::Font getTabButtonFont(juce::TabBarButton& button, float height) override
    {
        juce::ignoreUnused(button);
        juce::ignoreUnused(height);
        return withDefaultMetrics(makeUiFont(juce::Font::bold));
    }
};

Mx6LookAndFeel& getMx6LookAndFeel()
{
    static Mx6LookAndFeel lookAndFeel;
    return lookAndFeel;
}

class BoxTextButton final : public juce::TextButton
{
public:
    explicit BoxTextButton(const juce::Colour accent)
        : accentColour(accent)
    {
    }

    void setChangedState(const bool shouldBeChanged)
    {
        if (changedState == shouldBeChanged)
            return;

        changedState = shouldBeChanged;
        repaint();
    }

    void paintButton(juce::Graphics& graphics, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsHighlighted);

        const auto fill = shouldDrawButtonAsDown ? uiGrey700 : uiGrey800;
        const auto outline = changedState ? uiChanged : (getToggleState() ? accentColour : uiGrey500);
        const auto textColour = uiWhite;

        graphics.setColour(fill);
        graphics.fillRect(getLocalBounds());

        graphics.setColour(outline);
        graphics.drawRect(getLocalBounds(), 1);

        graphics.setColour(textColour);
        graphics.setFont(makeUiFont());
        graphics.drawFittedText(getButtonText(), getLocalBounds().reduced(3), juce::Justification::centred, 1, 1.0f);
    }

private:
    juce::Colour accentColour;
    bool changedState = false;
};

class ParameterControl final : public juce::Component
{
public:
    ParameterControl(juce::AudioProcessorValueTreeState& state,
                     const juce::String& parameterId,
                     const ControlSpec& spec,
                     const juce::Colour accent,
                     std::function<void()> onChangedStateChangeIn)
        : isToggle(spec.isToggle),
          tracksChangedState(spec.tracksChangedState),
          accentColour(accent),
          parameter(dynamic_cast<juce::RangedAudioParameter*>(state.getParameter(parameterId))),
          onChangedStateChange(std::move(onChangedStateChangeIn)),
          toggle(accent)
    {
        jassert(parameter != nullptr);

        if (isToggle)
        {
            toggle.setButtonText(spec.label);
            toggle.setClickingTogglesState(true);
            toggle.onStateChange = [this]
            {
                refreshChangedAppearance();
            };
            buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                state,
                parameterId,
                toggle);

            addAndMakeVisible(toggle);
        }
        else
        {
            title.setText(spec.label, juce::dontSendNotification);
            title.setJustificationType(juce::Justification::centredLeft);
            title.setColour(juce::Label::textColourId, uiWhite);
            title.setColour(juce::Label::backgroundColourId, uiGrey800);
            title.setColour(juce::Label::outlineColourId, uiGrey500);
            title.setFont(makeUiFont());
            title.setBorderSize({ 0, 6, 0, 2 });
            title.setMinimumHorizontalScale(1.0f);

            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            slider.setLookAndFeel(&getMx6LookAndFeel());
            slider.setInterceptsMouseClicks(false, false);
            slider.setAlpha(0.0f);
            slider.onResetRequest = [this]
            {
                resetParameterToDefault();
            };
            slider.onValueChange = [this]
            {
                if (valueBox != nullptr)
                    valueBox->repaint();

                refreshChangedAppearance();
            };
            slider.textFromValueFunction = [] (const double value)
            {
                return formatValueBoxText(value);
            };
            slider.valueFromTextFunction = [] (const juce::String& text)
            {
                return text.trim().getDoubleValue();
            };
            slider.setColour(juce::Slider::thumbColourId, uiBlack);
            slider.setColour(juce::Slider::trackColourId, uiBlack);
            slider.setColour(juce::Slider::backgroundColourId, uiBlack);
            slider.setColour(juce::Slider::textBoxBackgroundColourId, uiGrey800);
            slider.setColour(juce::Slider::textBoxTextColourId, uiWhite);
            slider.setColour(juce::Slider::textBoxOutlineColourId, uiGrey500);
            slider.setColour(juce::Slider::textBoxHighlightColourId, accentColour);
            sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                state,
                parameterId,
                slider);

            valueBox = std::make_unique<ValueBoxComponent>(slider, parameter);
            valueBox->onResetRequest = [this]
            {
                resetParameterToDefault();
            };
            applyCurrentAppearance();

            addAndMakeVisible(title);
            addAndMakeVisible(slider);
            addAndMakeVisible(*valueBox);
        }

        refreshChangedAppearance();
        applyCurrentAppearance();
        callbacksEnabled = true;
    }

    int getPreferredHeight() const noexcept
    {
        return isToggle ? 24 : 26;
    }

    bool isChangedFromDefault() const noexcept
    {
        return changedState;
    }

    ~ParameterControl() override
    {
        if (! isToggle)
            slider.setLookAndFeel(nullptr);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        if (isToggle)
        {
            toggle.setBounds(bounds);
            return;
        }

        auto row = bounds;
        title.setBounds(row.removeFromLeft(parameterNameWidth));
        row.removeFromLeft(parameterGap);
        slider.setBounds(row);

        if (valueBox != nullptr)
            valueBox->setBounds(row);
    }

private:
    void applyCurrentAppearance()
    {
        if (isToggle)
        {
            toggle.setChangedState(changedState);
            return;
        }

        const auto outline = changedState ? uiChanged : uiGrey500;
        const auto highlight = changedState ? uiChanged : accentColour;
        title.setColour(juce::Label::outlineColourId, outline);

        if (valueBox != nullptr)
        {
            valueBox->setOutlineColour(outline);
            valueBox->setHighlightColour(highlight);
        }
    }

    void resetParameterToDefault()
    {
        if (parameter == nullptr)
            return;

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
        parameter->endChangeGesture();
    }

    void refreshChangedAppearance()
    {
        if (! tracksChangedState)
        {
            if (changedState)
            {
                changedState = false;
                applyCurrentAppearance();

                if (callbacksEnabled && onChangedStateChange)
                    onChangedStateChange();
            }

            return;
        }

        const auto shouldBeChanged = parameter != nullptr
            && std::abs(parameter->getValue() - parameter->getDefaultValue()) > 1.0e-6f;

        if (changedState == shouldBeChanged)
            return;

        changedState = shouldBeChanged;
        applyCurrentAppearance();

        if (callbacksEnabled && onChangedStateChange)
            onChangedStateChange();
    }

    const bool isToggle = false;
    const bool tracksChangedState = true;
    const juce::Colour accentColour;
    juce::RangedAudioParameter* parameter = nullptr;
    std::function<void()> onChangedStateChange;
    bool callbacksEnabled = false;
    bool changedState = false;
    BoxTextButton toggle;
    ResettableParameterLabel title;
    ResettableValueSlider slider;
    std::unique_ptr<ValueBoxComponent> valueBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
};

class SectionComponent final : public juce::Component
{
public:
    SectionComponent(juce::AudioProcessorValueTreeState& state,
                     const std::function<juce::String(const char*)>& parameterIdProvider,
                     const SectionSpec& spec,
                     const juce::Colour accent,
                     std::function<void()> onLayoutChangeIn,
                     std::function<void()> onChangedStateChangeIn,
                     std::function<void(bool)> onExpandedChangedIn)
        : accentColour(accent),
          onLayoutChange(std::move(onLayoutChangeIn)),
          onChangedStateChange(std::move(onChangedStateChangeIn)),
          onExpandedChanged(std::move(onExpandedChangedIn)),
          staysExpandedOnSelfClick(spec.staysExpandedOnSelfClick),
          headerButton(accent)
    {
        headerButton.setButtonText(spec.title);
        headerButton.setClickingTogglesState(true);
        headerButton.setToggleState(spec.startsExpanded, juce::dontSendNotification);
        headerButton.onClick = [this]
        {
            if (! headerButton.getToggleState() && staysExpandedOnSelfClick)
                headerButton.setToggleState(true, juce::dontSendNotification);

            updateExpandedState();

            if (onExpandedChanged)
                onExpandedChanged(headerButton.getToggleState());

            if (onLayoutChange)
                onLayoutChange();
        };
        addAndMakeVisible(headerButton);

        controls.reserve(spec.controls.size());

        for (const auto& controlSpec : spec.controls)
        {
            auto control = std::make_unique<ParameterControl>(state,
                                                              parameterIdProvider(controlSpec.suffix),
                                                              controlSpec,
                                                              accentColour,
                                                              [this]
                                                              {
                                                                  refreshChangedAppearance();
                                                              });
            addAndMakeVisible(*control);
            controls.push_back(std::move(control));
        }

        updateExpandedState();
        refreshChangedAppearance();
        callbacksEnabled = true;
    }

    int getPreferredHeight() const
    {
        auto height = 22 + 8;

        if (headerButton.getToggleState())
        {
            for (const auto& control : controls)
                height += control->getPreferredHeight() + 6;
        }

        return height + 6;
    }

    bool isChangedFromDefault() const noexcept
    {
        return changedState;
    }

    bool isExpanded() const noexcept
    {
        return headerButton.getToggleState();
    }

    void setExpanded(const bool shouldBeExpanded)
    {
        if (headerButton.getToggleState() == shouldBeExpanded)
            return;

        headerButton.setToggleState(shouldBeExpanded, juce::dontSendNotification);
        updateExpandedState();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(uiGrey800);
        graphics.fillRect(getLocalBounds());

        graphics.setColour(uiGrey700);
        graphics.drawRect(getLocalBounds(), 1);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(4, 6);
        headerButton.setBounds(bounds.removeFromTop(22));
        bounds.removeFromTop(6);

        if (! headerButton.getToggleState())
            return;

        for (auto& control : controls)
        {
            const auto height = control->getPreferredHeight();
            control->setBounds(bounds.removeFromTop(height));
            bounds.removeFromTop(6);
        }
    }

private:
    void refreshChangedAppearance()
    {
        bool shouldBeChanged = false;

        for (const auto& control : controls)
        {
            if (control->isChangedFromDefault())
            {
                shouldBeChanged = true;
                break;
            }
        }

        if (changedState == shouldBeChanged)
            return;

        changedState = shouldBeChanged;
        headerButton.setChangedState(changedState);

        if (callbacksEnabled && onChangedStateChange)
            onChangedStateChange();
    }

    void updateExpandedState()
    {
        const auto expanded = headerButton.getToggleState();

        for (auto& control : controls)
            control->setVisible(expanded);
    }

    const juce::Colour accentColour;
    std::function<void()> onLayoutChange;
    std::function<void()> onChangedStateChange;
    std::function<void(bool)> onExpandedChanged;
    const bool staysExpandedOnSelfClick = false;
    bool callbacksEnabled = false;
    bool changedState = false;
    BoxTextButton headerButton;
    std::vector<std::unique_ptr<ParameterControl>> controls;
};

class BandPageComponent final : public juce::Component
{
public:
    BandPageComponent(juce::AudioProcessorValueTreeState& state,
                      const std::function<juce::String(const char*)>& parameterIdProvider,
                      const juce::Colour accent,
                      std::function<void()> onChangedStateChangeIn)
        : onChangedStateChange(std::move(onChangedStateChangeIn)),
          halfWave(state,
                   parameterIdProvider,
                   halfWaveSection,
                   accent,
                   [this] { resized(); },
                   [this] { refreshChangedAppearance(); },
                   [this] (bool expanded) { handleSectionExpanded(0, expanded); }),
          dm(state,
             parameterIdProvider,
             dmSection,
             accent,
             [this] { resized(); },
             [this] { refreshChangedAppearance(); },
             [this] (bool expanded) { handleSectionExpanded(1, expanded); }),
          ff(state,
             parameterIdProvider,
             ffSection,
             accent,
             [this] { resized(); },
             [this] { refreshChangedAppearance(); },
             [this] (bool expanded) { handleSectionExpanded(2, expanded); }),
          global(state,
                 parameterIdProvider,
                 globalSection,
                 accent,
                 [this] { resized(); },
                 [this] { refreshChangedAppearance(); },
                 [this] (bool expanded) { handleSectionExpanded(3, expanded); })
    {
        addAndMakeVisible(halfWave);
        addAndMakeVisible(dm);
        addAndMakeVisible(ff);
        addAndMakeVisible(global);
        refreshChangedAppearance();
        callbacksEnabled = true;
    }

    bool isChangedFromDefault() const noexcept
    {
        return changedState;
    }

    int getPreferredHeight() const
    {
        return 8
            + halfWave.getPreferredHeight()
            + 6
            + dm.getPreferredHeight()
            + 6
            + ff.getPreferredHeight()
            + 6
            + global.getPreferredHeight();
    }

    void paint(juce::Graphics& graphics) override
    {
        juce::ignoreUnused(graphics);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(0, 4);

        auto sectionBounds = bounds.removeFromTop(halfWave.getPreferredHeight());
        halfWave.setBounds(sectionBounds);
        bounds.removeFromTop(6);
        sectionBounds = bounds.removeFromTop(dm.getPreferredHeight());
        dm.setBounds(sectionBounds);
        bounds.removeFromTop(6);
        sectionBounds = bounds.removeFromTop(ff.getPreferredHeight());
        ff.setBounds(sectionBounds);
        bounds.removeFromTop(6);
        sectionBounds = bounds.removeFromTop(global.getPreferredHeight());
        global.setBounds(sectionBounds);
    }

private:
    void handleSectionExpanded(const int sectionIndex, const bool expanded)
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

    void refreshChangedAppearance()
    {
        const auto shouldBeChanged = halfWave.isChangedFromDefault()
            || dm.isChangedFromDefault()
            || ff.isChangedFromDefault()
            || global.isChangedFromDefault();

        if (changedState == shouldBeChanged)
            return;

        changedState = shouldBeChanged;
        repaint();

        if (callbacksEnabled && onChangedStateChange)
            onChangedStateChange();
    }

    std::function<void()> onChangedStateChange;
    bool callbacksEnabled = false;
    bool changedState = false;
    SectionComponent halfWave;
    SectionComponent dm;
    SectionComponent ff;
    SectionComponent global;
};

class FullbandPageComponent final : public juce::Component
{
public:
    FullbandPageComponent(juce::AudioProcessorValueTreeState& state,
                          const std::function<juce::String(const char*)>& parameterIdProvider)
        : fullband(state,
                   parameterIdProvider,
                   fullbandSection,
                   uiAccent,
                   [this] { resized(); },
                   [] {},
                   [] (bool) {})
    {
        addAndMakeVisible(fullband);
    }

    int getPreferredHeight() const
    {
        return 8 + fullband.getPreferredHeight();
    }

    void paint(juce::Graphics& graphics) override
    {
        juce::ignoreUnused(graphics);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(0, 4);
        fullband.setBounds(bounds.removeFromTop(fullband.getPreferredHeight()));
    }

private:
    SectionComponent fullband;
};
} // namespace

Mx6AudioProcessorEditor::Mx6AudioProcessorEditor(Mx6AudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor(&processorToEdit)
{
    auto& valueTreeState = processorToEdit.getValueTreeState();
    auto* singleBandParameter = dynamic_cast<juce::RangedAudioParameter*>(valueTreeState.getParameter(makeSingleBandModeParameterId()));
    jassert(singleBandParameter != nullptr);
    singleBandModeParameter = singleBandParameter;

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(valueTreeState.getParameter(makeSoloParameterId(bandIndex)));
        jassert(parameter != nullptr);
        soloParameters[bandIndex] = parameter;

        auto button = std::make_unique<BoxTextButton>(uiAccent);
        button->setButtonText(makeBandName(bandIndex));
        button->setClickingTogglesState(false);
        button->onClick = [this, bandIndex] { selectBand(bandIndex); };
        addAndMakeVisible(*button);
        monitorButtons[bandIndex] = std::move(button);

        auto page = std::make_unique<BandPageComponent>(
            valueTreeState,
            [bandIndex] (const char* suffix)
            {
                return makeBandParameterId(bandIndex, suffix);
            },
            bandColour(bandIndex),
            [this]
            {
                updatePageChangedIndicators();
            });
        addAndMakeVisible(*page);
        bandPages[bandIndex] = std::move(page);
    }

    singleBandPage = std::make_unique<BandPageComponent>(
        valueTreeState,
        [] (const char* suffix)
        {
            return makeSingleParameterId(suffix);
        },
        uiAccent,
        [this]
        {
            updatePageChangedIndicators();
        });
    addAndMakeVisible(*singleBandPage);

    size_t activeSoloCount = 0;

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        if (soloParameters[bandIndex] != nullptr && soloParameters[bandIndex]->getValue() >= 0.5f)
        {
            if (activeSoloCount == 0)
                visibleBandIndex = bandIndex;

            ++activeSoloCount;
        }
    }

    singleBandMode = singleBandModeParameter != nullptr && singleBandModeParameter->getValue() >= 0.5f;
    allBandsActive = singleBandMode || activeSoloCount != 1;

    auto allButton = std::make_unique<BoxTextButton>(uiAccent);
    allButton->setButtonText("A");
    allButton->setClickingTogglesState(false);
    allButton->onClick = [this] { setAllBandsMonitoring(); };
    addAndMakeVisible(*allButton);
    monitorButtons[numBands] = std::move(allButton);

    allBandsPage = std::make_unique<FullbandPageComponent>(
        valueTreeState,
        [] (const char* suffix)
        {
            return makeFullbandParameterId(suffix);
        });
    addAndMakeVisible(*allBandsPage);

    auto modeButton = std::make_unique<BoxTextButton>(uiAccent);
    modeButton->setButtonText(singleBandMode ? "S" : "M");
    modeButton->setClickingTogglesState(false);
    modeButton->onClick = [this] { toggleProcessingMode(); };
    addAndMakeVisible(*modeButton);
    monitorButtons[numBands + 1] = std::move(modeButton);

    footerLabel.setText("MX6 by MIXOLVE", juce::dontSendNotification);
    footerLabel.setJustificationType(juce::Justification::centred);
    footerLabel.setColour(juce::Label::textColourId, uiWhite);
    footerLabel.setFont(makeUiFont());
    addAndMakeVisible(footerLabel);

    updatePageChangedIndicators();
    updateMonitorButtons();
    updateBandPageVisibility();

    setResizable(false, false);
    setResizeLimits(fixedEditorWidth, fixedEditorHeight, fixedEditorWidth, fixedEditorHeight);
    setSize(fixedEditorWidth, fixedEditorHeight);
}

Mx6AudioProcessorEditor::~Mx6AudioProcessorEditor() = default;

void Mx6AudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(uiGrey800);
}

void Mx6AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(editorInsetX);
    bounds.removeFromRight(editorInsetX);
    bounds.removeFromBottom(editorInsetBottom);
    auto monitorRow = bounds.removeFromTop(22);
    const auto buttonCount = static_cast<int>(monitorButtons.size());
    const auto totalGapWidth = monitorButtonGap * (buttonCount - 1);
    const auto baseButtonWidth = (monitorRow.getWidth() - totalGapWidth) / buttonCount;
    auto remainder = (monitorRow.getWidth() - totalGapWidth) - (baseButtonWidth * buttonCount);

    for (auto& button : monitorButtons)
    {
        if (button == nullptr)
            continue;

        const auto buttonWidth = baseButtonWidth + (remainder > 0 ? 1 : 0);
        button->setBounds(monitorRow.removeFromLeft(buttonWidth));
        monitorRow.removeFromLeft(monitorButtonGap);
        remainder = juce::jmax(0, remainder - 1);
    }

    bounds.removeFromTop(4);

    auto footerBounds = bounds.removeFromBottom(footerHeight);
    footerLabel.setBounds(footerBounds.translated(0, -3));
    bounds.removeFromBottom(4);

    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        if (auto* page = bandPages[bandIndex].get())
        {
            if (auto* bandPage = dynamic_cast<BandPageComponent*>(page))
                page->setBounds(bounds.withHeight(juce::jmin(bounds.getHeight(), bandPage->getPreferredHeight())));
            else
                page->setBounds(bounds);
        }
    }

    if (auto* page = allBandsPage.get())
    {
        if (auto* fullbandPage = dynamic_cast<FullbandPageComponent*>(page))
            page->setBounds(bounds.withHeight(juce::jmin(bounds.getHeight(), fullbandPage->getPreferredHeight())));
        else
            page->setBounds(bounds);
    }

    if (auto* page = singleBandPage.get())
    {
        if (auto* bandPage = dynamic_cast<BandPageComponent*>(page))
            page->setBounds(bounds.withHeight(juce::jmin(bounds.getHeight(), bandPage->getPreferredHeight())));
        else
            page->setBounds(bounds);
    }
}

void Mx6AudioProcessorEditor::selectBand(const size_t bandIndex)
{
    if (singleBandMode)
        return;

    visibleBandIndex = juce::jmin(bandIndex, numBands - 1);
    allBandsActive = false;
    updateMonitorButtons();
    updateBandPageVisibility();
    syncMonitorParameters();
}

void Mx6AudioProcessorEditor::toggleProcessingMode()
{
    if (singleBandMode)
    {
        singleBandMode = false;
        allBandsActive = true;
    }
    else
    {
        singleBandMode = true;
        allBandsActive = true;
    }

    updateMonitorButtons();
    updateBandPageVisibility();
    syncMonitorParameters();
}

void Mx6AudioProcessorEditor::setAllBandsMonitoring()
{
    singleBandMode = false;
    allBandsActive = true;
    updateMonitorButtons();
    updateBandPageVisibility();
    syncMonitorParameters();
}

void Mx6AudioProcessorEditor::syncMonitorParameters()
{
    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        auto* parameter = soloParameters[bandIndex];

        if (parameter == nullptr)
            continue;

        const auto enabled = ! singleBandMode && ! allBandsActive && bandIndex == visibleBandIndex;
        const auto newValue = parameter->convertTo0to1(enabled ? 1.0f : 0.0f);

        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(newValue);
        parameter->endChangeGesture();
    }

    if (singleBandModeParameter != nullptr)
    {
        const auto newValue = singleBandModeParameter->convertTo0to1(singleBandMode ? 1.0f : 0.0f);
        singleBandModeParameter->beginChangeGesture();
        singleBandModeParameter->setValueNotifyingHost(newValue);
        singleBandModeParameter->endChangeGesture();
    }
}

void Mx6AudioProcessorEditor::updateMonitorButtons()
{
    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        if (auto* button = monitorButtons[bandIndex].get())
            button->setToggleState(! singleBandMode && ! allBandsActive && bandIndex == visibleBandIndex, juce::dontSendNotification);
    }

    if (auto* button = monitorButtons[numBands].get())
        button->setToggleState(! singleBandMode && allBandsActive, juce::dontSendNotification);

    if (auto* button = monitorButtons[numBands + 1].get())
    {
        button->setButtonText(singleBandMode ? "S" : "M");
        button->setToggleState(singleBandMode, juce::dontSendNotification);
    }
}

void Mx6AudioProcessorEditor::updateBandPageVisibility()
{
    for (size_t bandIndex = 0; bandIndex < numBands; ++bandIndex)
    {
        if (auto* page = bandPages[bandIndex].get())
            page->setVisible(! singleBandMode && ! allBandsActive && bandIndex == visibleBandIndex);
    }

    if (auto* page = allBandsPage.get())
        page->setVisible(! singleBandMode && allBandsActive);

    if (auto* page = singleBandPage.get())
        page->setVisible(singleBandMode);
}

void Mx6AudioProcessorEditor::updatePageChangedIndicators()
{
    for (auto& monitorButton : monitorButtons)
    {
        if (auto* button = dynamic_cast<BoxTextButton*>(monitorButton.get()))
            button->setChangedState(false);
    }
}
