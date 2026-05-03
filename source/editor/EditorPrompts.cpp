#include "EditorPrompts.h"

#include "EditorTheme.h"

#include <utility>
#include <vector>

namespace mxe::editor
{
namespace
{
constexpr int promptPanelPadding = 8;
constexpr int promptItemGap = 4;
constexpr int promptEditorHeight = 26;

struct MarkdownLink
{
    juce::String label;
    juce::String url;
};

juce::String getDisplayNameFromUrl(const juce::String& urlText)
{
    auto displayName = urlText.trim();

    if (displayName.startsWithIgnoreCase("https://"))
        displayName = displayName.substring(8);
    else if (displayName.startsWithIgnoreCase("http://"))
        displayName = displayName.substring(7);

    return displayName.upToFirstOccurrenceOf("/", false, false).trim();
}

MarkdownLink parseMarkdownLinkLine(const juce::String& line)
{
    MarkdownLink link;
    const auto trimmed = line.trim();

    if (trimmed.startsWithChar('['))
    {
        const auto labelEnd = trimmed.indexOfChar(']');
        const auto openParen = trimmed.indexOfChar('(');
        const auto closeParen = trimmed.lastIndexOfChar(')');

        if (labelEnd > 1 && openParen > labelEnd && closeParen > openParen)
        {
            link.label = trimmed.substring(1, labelEnd).trim();
            link.url = trimmed.substring(openParen + 1, closeParen).trim();
            return link;
        }
    }

    if (trimmed.startsWithIgnoreCase("https://") || trimmed.startsWithIgnoreCase("http://"))
    {
        link.label = getDisplayNameFromUrl(trimmed);
        link.url = trimmed;
    }

    return link;
}

struct MarkdownBlock
{
    enum class Kind
    {
        text,
        link,
        spacer
    };

    Kind kind = Kind::text;
    juce::String text;
    juce::String url;
    int headingLevel = 0;
};

MarkdownBlock parseMarkdownBlock(const juce::String& line)
{
    MarkdownBlock block;
    const auto trimmed = line.trim();

    if (trimmed.isEmpty())
    {
        block.kind = MarkdownBlock::Kind::spacer;
        return block;
    }

    int headingLevel = 0;

    while (headingLevel < trimmed.length() && trimmed[headingLevel] == '#')
        ++headingLevel;

    if (headingLevel > 0
        && headingLevel <= 6
        && (headingLevel == trimmed.length()
            || trimmed[headingLevel] == ' '
            || trimmed[headingLevel] == '\t'))
    {
        block.headingLevel = headingLevel;
        block.text = trimmed.substring(headingLevel).trim();
        return block;
    }

    const auto link = parseMarkdownLinkLine(trimmed);

    if (link.url.isNotEmpty())
    {
        block.kind = MarkdownBlock::Kind::link;
        block.text = link.label;
        block.url = link.url;
        return block;
    }

    block.text = trimmed;
    return block;
}

std::vector<MarkdownBlock> parseMarkdownBlocks(const juce::String& markdownText)
{
    std::vector<MarkdownBlock> blocks;
    juce::String paragraphText;
    bool lastWasSpacer = false;

    const auto flushParagraph = [&]
    {
        if (paragraphText.isEmpty())
            return;

        MarkdownBlock block;
        block.text = paragraphText.trimEnd();
        blocks.push_back(std::move(block));
        paragraphText.clear();
        lastWasSpacer = false;
    };

    for (const auto& rawLine : juce::StringArray::fromLines(markdownText))
    {
        const auto parsed = parseMarkdownBlock(rawLine);

        if (parsed.kind == MarkdownBlock::Kind::spacer)
        {
            flushParagraph();

            if (! blocks.empty() && ! lastWasSpacer)
            {
                MarkdownBlock spacer;
                spacer.kind = MarkdownBlock::Kind::spacer;
                blocks.push_back(std::move(spacer));
                lastWasSpacer = true;
            }

            continue;
        }

        if (parsed.kind == MarkdownBlock::Kind::text && parsed.headingLevel == 0)
        {
            if (paragraphText.isNotEmpty())
                paragraphText << '\n';

            paragraphText << parsed.text;
            lastWasSpacer = false;
            continue;
        }

        flushParagraph();
        blocks.push_back(parsed);
        lastWasSpacer = false;
    }

    flushParagraph();

    while (! blocks.empty() && blocks.front().kind == MarkdownBlock::Kind::spacer)
        blocks.erase(blocks.begin());

    while (! blocks.empty() && blocks.back().kind == MarkdownBlock::Kind::spacer)
        blocks.pop_back();

    if (blocks.empty())
    {
        MarkdownBlock fallback;
        fallback.kind = MarkdownBlock::Kind::link;
        fallback.text = "mixolve.cc";
        fallback.url = "https://mixolve.cc/";
        blocks.push_back(std::move(fallback));
    }

    return blocks;
}

class MarkdownRowComponent : public juce::Component
{
public:
    ~MarkdownRowComponent() override = default;

    virtual int getPreferredHeight(int width) const = 0;
};

class MarkdownTextRow final : public MarkdownRowComponent
{
public:
    MarkdownTextRow(juce::String rowText, juce::Font rowFont, juce::Colour rowColour)
        : text(std::move(rowText)),
          font(std::move(rowFont)),
          colour(rowColour)
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
        setInterceptsMouseClicks(false, false);
    }

    int getPreferredHeight(const int width) const override
    {
        updateLayout(width);
        return juce::jmax(1, juce::roundToInt(layout.getHeight()));
    }

    void paint(juce::Graphics& graphics) override
    {
        updateLayout(getWidth());
        graphics.setColour(colour);
        layout.draw(graphics, getLocalBounds().toFloat());
    }

private:
    void updateLayout(const int width) const
    {
        const auto contentWidth = juce::jmax(1, width);

        if (cachedWidth == contentWidth)
            return;

        juce::AttributedString attributed(text);
        attributed.setFont(font);
        attributed.setColour(colour);
        attributed.setJustification(juce::Justification::centred);
        attributed.setWordWrap(juce::AttributedString::byWord);

        layout.createLayout(attributed, static_cast<float>(contentWidth));
        cachedWidth = contentWidth;
    }

    juce::String text;
    juce::Font font;
    juce::Colour colour;
    mutable juce::TextLayout layout;
    mutable int cachedWidth = -1;
};

class MarkdownLinkRow final : public MarkdownRowComponent
{
public:
    MarkdownLinkRow(juce::String text, juce::String urlText)
        : linkButton(std::move(text), juce::URL(urlText))
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
        setInterceptsMouseClicks(false, true);

        const auto linkFont = juce::Font(makeUiFont(juce::Font::underlined, 22.0f));
        const auto linkHeight = juce::jmax(1, juce::roundToInt(linkFont.getHeight()));

        linkButton.setFont(linkFont, false);
        linkButton.setJustificationType(juce::Justification::centred);
        linkButton.setColour(juce::HyperlinkButton::textColourId, uiAccent);
        linkButton.setMouseClickGrabsKeyboardFocus(false);
        linkButton.setWantsKeyboardFocus(false);
        linkButton.setSize(1, linkHeight);
        linkButton.changeWidthToFitText();
        addAndMakeVisible(linkButton);
    }

    int getPreferredHeight(int) const override
    {
        return juce::jmax(1, juce::roundToInt(linkButton.getHeight()));
    }

    void resized() override
    {
        linkButton.setBounds(linkButton.getBounds().withCentre(getLocalBounds().getCentre()));
    }

private:
    juce::HyperlinkButton linkButton;
};

class MarkdownSpacerRow final : public MarkdownRowComponent
{
public:
    explicit MarkdownSpacerRow(const int spacerHeightIn)
        : spacerHeight(spacerHeightIn)
    {
        setOpaque(false);
        setWantsKeyboardFocus(false);
        setMouseClickGrabsKeyboardFocus(false);
        setInterceptsMouseClicks(false, false);
    }

    int getPreferredHeight(int) const override
    {
        return juce::jmax(0, spacerHeight);
    }

private:
    int spacerHeight = 0;
};

class MarkdownContentView final : public juce::Component
{
public:
    explicit MarkdownContentView(juce::String markdownText)
    {
        setOpaque(false);
        setInterceptsMouseClicks(false, true);
        setMarkdownText(std::move(markdownText));
    }

    void setMarkdownText(juce::String markdownText)
    {
        rows.clear();

        const auto blocks = parseMarkdownBlocks(markdownText);
        rows.reserve(blocks.size());

        for (const auto& block : blocks)
        {
            if (block.kind == MarkdownBlock::Kind::spacer)
            {
                rows.push_back(std::make_unique<MarkdownSpacerRow>(promptItemGap));
            }
            else if (block.kind == MarkdownBlock::Kind::link)
            {
                rows.push_back(std::make_unique<MarkdownLinkRow>(block.text, block.url));
            }
            else
            {
                const auto font = juce::Font(block.headingLevel > 0 ? makeUiFont(juce::Font::bold)
                                                                    : makeUiFont());

                rows.push_back(std::make_unique<MarkdownTextRow>(block.text, font, uiWhite));
            }

            addAndMakeVisible(*rows.back());
        }

        resized();
        repaint();
    }

    int getContentHeight(const int width) const
    {
        const auto contentWidth = juce::jmax(1, width - (contentPadding * 2));

        if (rows.empty())
            return contentPadding * 2;

        return getRowsHeight(contentWidth) + (contentPadding * 2);
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().reduced(contentPadding);
        const auto contentWidth = bounds.getWidth();
        const auto rowsHeight = getRowsHeight(contentWidth);
        auto y = bounds.getY() + juce::jmax(0, (bounds.getHeight() - rowsHeight) / 2);

        for (size_t index = 0; index < rows.size(); ++index)
        {
            const auto rowHeight = rows[index]->getPreferredHeight(contentWidth);
            rows[index]->setBounds(bounds.getX(), y, contentWidth, rowHeight);
            y += rowHeight;

            if (index + 1 < rows.size())
                y += promptItemGap;
        }
    }

private:
    int getRowsHeight(const int width) const
    {
        const auto contentWidth = juce::jmax(1, width);
        auto totalHeight = 0;

        for (size_t index = 0; index < rows.size(); ++index)
        {
            totalHeight += rows[index]->getPreferredHeight(contentWidth);

            if (index + 1 < rows.size())
                totalHeight += promptItemGap;
        }

        return totalHeight;
    }

    std::vector<std::unique_ptr<MarkdownRowComponent>> rows;
    int contentPadding = promptPanelPadding;
};

class FloatingTextPrompt final : public juce::Component
{
public:
    using CommitCallback = std::function<bool(const juce::String&)>;
    using DismissCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;

    FloatingTextPrompt(juce::String currentText,
                       CommitCallback commitCallback,
                       DismissCallback dismissCallback,
                     CloseCallback closeCallback,
                     std::function<juce::Rectangle<int>()> anchorBoundsProviderIn)
        : onCommit(std::move(commitCallback)),
          onDismiss(std::move(dismissCallback)),
            onClose(std::move(closeCallback)),
            anchorBoundsProvider(std::move(anchorBoundsProviderIn))
    {
        setOpaque(false);
        setWantsKeyboardFocus(true);
        setMouseClickGrabsKeyboardFocus(false);
        setInterceptsMouseClicks(true, true);

        textEditor.setFont(makeUiFont());
        textEditor.setWantsKeyboardFocus(true);
        textEditor.setMouseClickGrabsKeyboardFocus(true);
        textEditor.setPopupMenuEnabled(true);
        textEditor.setJustification(juce::Justification::centred);
        textEditor.setColour(juce::TextEditor::textColourId, uiWhite);
        textEditor.setColour(juce::TextEditor::backgroundColourId, uiGrey800);
        textEditor.setColour(juce::TextEditor::outlineColourId, uiGrey500);
        textEditor.setColour(juce::TextEditor::focusedOutlineColourId, uiAccent);
        textEditor.setColour(juce::TextEditor::highlightColourId, uiAccent);
        textEditor.setColour(juce::TextEditor::highlightedTextColourId, uiWhite);
        textEditor.setText(std::move(currentText), false);
        textEditor.setReturnKeyStartsNewLine(false);
        textEditor.onReturnKey = [this] { commit(); };
        textEditor.onEscapeKey = [this] { cancel(); };
        addAndMakeVisible(textEditor);

        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<FloatingTextPrompt>(this)]
                                        {
                                            if (safeThis != nullptr)
                                                safeThis->grabEditorFocus();
                                        });
    }

    void grabEditorFocus()
    {
        textEditor.grabKeyboardFocus();
        textEditor.selectAll();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            cancel();
            return true;
        }

        return false;
    }

    void focusGained(FocusChangeType) override
    {
        grabEditorFocus();
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(juce::Colours::black.withAlpha(0.55f));
        graphics.fillAll();
    }

    void resized() override
    {
        if (anchorBoundsProvider != nullptr)
        {
            const auto anchorBounds = anchorBoundsProvider();

            if (! anchorBounds.isEmpty())
            {
                textEditor.setBounds(anchorBounds);
                return;
            }
        }

        const auto visibleBounds = getLocalBounds();
        const auto availableWidth = juce::jmax(0, visibleBounds.getWidth() - (promptPanelPadding * 2));
        const auto promptWidth = juce::jmax(1, availableWidth);

        auto promptBounds = juce::Rectangle<int>(promptWidth, promptEditorHeight)
                                .withCentre(visibleBounds.getCentre())
                                .constrainedWithin(visibleBounds.reduced(promptPanelPadding));

        textEditor.setBounds(promptBounds);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (! textEditor.getBounds().contains(event.getPosition()))
            cancel();
    }

private:
    void requestClose()
    {
        if (closePending)
            return;

        closePending = true;

        if (onDismiss != nullptr)
            onDismiss();

        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<FloatingTextPrompt>(this)]
                                        {
                                            if (safeThis == nullptr || safeThis->onClose == nullptr)
                                                return;

                                            safeThis->onClose();
                                        });
    }

    void commit()
    {
        if (closePending)
            return;

        const auto enteredText = textEditor.getText().trim();

        if (enteredText.isEmpty())
        {
            textEditor.selectAll();
            textEditor.grabKeyboardFocus();
            return;
        }

        if (onCommit != nullptr && ! onCommit(enteredText))
        {
            textEditor.selectAll();
            textEditor.grabKeyboardFocus();
            return;
        }

        requestClose();
    }

    void cancel()
    {
        requestClose();
    }

    juce::TextEditor textEditor;
    CommitCallback onCommit;
    DismissCallback onDismiss;
    CloseCallback onClose;
    std::function<juce::Rectangle<int>()> anchorBoundsProvider;
    bool closePending = false;
};

class FloatingInfoPrompt final : public juce::Component
{
public:
    FloatingInfoPrompt(juce::String markdownText,
                       std::function<juce::Rectangle<int>()> anchorBoundsProviderIn,
                       std::function<void()> closeCallback)
        : anchorBoundsProvider(std::move(anchorBoundsProviderIn)),
          markdownContent(std::move(markdownText)),
          onClose(std::move(closeCallback))
    {
        setOpaque(false);
        setWantsKeyboardFocus(true);
        setMouseClickGrabsKeyboardFocus(false);
        setInterceptsMouseClicks(true, true);

        addAndMakeVisible(markdownContent);
    }

    void paint(juce::Graphics& graphics) override
    {
        graphics.setColour(juce::Colours::black.withAlpha(0.55f));
        graphics.fillAll();

        graphics.setColour(uiGrey700);
        graphics.fillRect(panelBounds);

        graphics.setColour(uiGrey500);
        graphics.drawRect(panelBounds, 1);
    }

    void resized() override
    {
        const auto visibleBounds = getVisibleBounds();
        const auto availableWidth = juce::jmax(0, visibleBounds.getWidth() - (promptPanelPadding * 2));
        const auto availableHeight = juce::jmax(0, visibleBounds.getHeight() - (promptPanelPadding * 2));
        auto desiredWidth = availableWidth;

        if (anchorBoundsProvider != nullptr)
        {
            const auto anchorBounds = anchorBoundsProvider();

            if (! anchorBounds.isEmpty())
                desiredWidth = juce::jmin(anchorBounds.getWidth(), availableWidth);
        }

        const auto contentWidth = juce::jmax(0, desiredWidth - (promptPanelPadding * 2));
        const auto desiredHeight = juce::jmin(markdownContent.getContentHeight(contentWidth) + (promptPanelPadding * 2),
                                              availableHeight);

        panelBounds = juce::Rectangle<int>(desiredWidth, desiredHeight)
                          .withCentre(visibleBounds.getCentre())
                          .constrainedWithin(visibleBounds.reduced(promptPanelPadding));

        markdownContent.setBounds(panelBounds.reduced(promptPanelPadding));
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (! panelBounds.contains(event.getPosition()))
            requestClose();
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            requestClose();
            return true;
        }

        return false;
    }

private:
    juce::Rectangle<int> getVisibleBounds() const
    {
        auto bounds = getLocalBounds();

#if JUCE_IOS || JUCE_ANDROID
        if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds()))
            bounds = display->safeAreaInsets.subtractedFrom(display->keyboardInsets.subtractedFrom(bounds));
#endif

        return bounds;
    }

    void requestClose()
    {
        if (closePending)
            return;

        closePending = true;

        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<FloatingInfoPrompt>(this)]
                                        {
                                            if (safeThis != nullptr && safeThis->onClose != nullptr)
                                                safeThis->onClose();
                                        });
    }

    std::function<juce::Rectangle<int>()> anchorBoundsProvider;
    juce::Rectangle<int> panelBounds;
    MarkdownContentView markdownContent;
    std::function<void()> onClose;
    bool closePending = false;
};
} // namespace

std::unique_ptr<juce::Component> makeInfoPromptOverlay(juce::String markdownText,
                                                       std::function<juce::Rectangle<int>()> anchorBoundsProvider,
                                                       std::function<void()> onClose)
{
    return std::make_unique<FloatingInfoPrompt>(std::move(markdownText),
                                                std::move(anchorBoundsProvider),
                                                std::move(onClose));
}

std::unique_ptr<juce::Component> makeTextPromptOverlay(juce::String currentText,
                                                       std::function<bool(const juce::String&)> onCommit,
                                                       std::function<void()> onDismiss,
                                                       std::function<void()> onClose,
                                                       std::function<juce::Rectangle<int>()> anchorBoundsProvider)
{
    return std::make_unique<FloatingTextPrompt>(std::move(currentText),
                                                std::move(onCommit),
                                                std::move(onDismiss),
                                                std::move(onClose),
                                                std::move(anchorBoundsProvider));
}
} // namespace mxe::editor
