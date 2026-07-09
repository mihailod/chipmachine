#include "AbstractCellView.h"

namespace arkostracker 
{

const int AbstractCellView::borderWidth = 6;
const float AbstractCellView::textColorInterpolation = 0.4F;
const float AbstractCellView::cursorColorInterpolation = 0.7F;
const float AbstractCellView::selectionAlpha = 0.2F;

AbstractCellView::AbstractCellView(const int pViewRank, const juce::Font& pFont, const juce::Font& pDoubleFont, const CellColors& pCellColors) noexcept :
        useDoubleFont(false),
        viewRank(pViewRank),
        cellColors(std::make_unique<CellColors>(pCellColors)),
        font(pFont),
        doubleFont(pDoubleFont),
        fontHeightUsedCalculatingWidth1Char(-1.0F),        // Lazily calculated.
        cachedWidth1Char(-1)                            // Idem.
{
    setOpaque(true);
}

int AbstractCellView::calculateCharWidth(const juce::Font& font) noexcept
{
    return font.getStringWidth("X");        // Supposedly the largest character.
}

int AbstractCellView::getWidth1Char() const noexcept
{
    // Font height is the same?
    const auto fontHeight = font.getHeight();
    if (juce::exactlyEqual(fontHeightUsedCalculatingWidth1Char, fontHeight)) {
        jassert(cachedWidth1Char > 0);
        return cachedWidth1Char;
    }

    // Calculates the width.
    fontHeightUsedCalculatingWidth1Char = fontHeight;
    cachedWidth1Char = calculateCharWidth(font);

    return cachedWidth1Char;
}

void AbstractCellView::updateColors(CellColors newCellColors, const bool mustRepaint) noexcept
{
    cellColors = std::make_unique<CellColors>(newCellColors);
    if (mustRepaint) {
        repaint();
    }
}

void AbstractCellView::displayChar(juce::Graphics& g, const int letterIndex, const int baselineY, const int height, const juce::String& letter,
                                   juce::Colour letterColour, const bool isCursorOnLetter, const bool isCursorOnLine, const OptionalBool isPrimaryHighlight) const noexcept
{
    const auto width1Char = getWidth1Char();
    const auto x = (letterIndex * width1Char) + borderWidth;

    // Fills the background first. Even if no letter actually displayed, the cursor must still be shown.
    if (isCursorOnLetter) {
        g.setColour(cellColors->cursorBackgroundColor);
        g.fillRect(x, 0, width1Char, height);
    }

    // Optimization. The letter may be absent. Can happen in Special tracks, as we don't show the 00.
    if (letter.isEmpty()) {
        return;
    }

    // What color for the text?
    letterColour = determineBaseTextColor(letterColour, isCursorOnLetter, isCursorOnLine, isPrimaryHighlight);

    g.setColour(letterColour);
    g.drawSingleLineText(letter, x, baselineY);
}

juce::Colour AbstractCellView::determineBackgroundColor(const bool showSomething, const OptionalBool primaryHighlight, const bool isCursorPresentOnLine,
    const bool isPlayedLine) const noexcept
{
    auto currentBackgroundColor = cellColors->backgroundColor;

    // Anything to display? If not, fills the whole and, exits.
    if (!showSomething) {
        jassert(currentBackgroundColor.isOpaque());     // Background color should be opaque!
        return currentBackgroundColor;
    }

    // The background color. Can be altered from the highlight lines, and if there is a cursor.
    if (primaryHighlight.isPresent()) {
        // Chooses the primary or secondary highlight.
        currentBackgroundColor = primaryHighlight.getValue() ? cellColors->linePrimaryHighlightBackgroundColor : cellColors->lineSecondaryHighlightBackgroundColor;
    }
    // Cursor line?
    if (isCursorPresentOnLine) {
        currentBackgroundColor = currentBackgroundColor.interpolatedWith(cellColors->cursorLineInterpolatedBackgroundColor, 0.5F);
    }
    // Played line?
    if (isPlayedLine) {
        currentBackgroundColor = currentBackgroundColor.interpolatedWith(cellColors->playedLineBackgroundColor, 0.5F);
    }

    jassert(currentBackgroundColor.isOpaque());     // Background color should be opaque!

    return currentBackgroundColor;
}

juce::Colour AbstractCellView::determineBaseTextColor(juce::Colour letterColor, const bool isCursorOnLetter, const bool isCursorOnLine,
    const OptionalBool isPrimaryHighlight) const noexcept {
    // Cursor on it?
    if (isCursorOnLetter) {
        // Highlights the letter.
        letterColor = letterColor.interpolatedWith(cellColors->cursorTextHighlightColor, cursorColorInterpolation);
    }

    // Interpolates the letter with the possible highlight.
    if (isPrimaryHighlight.isPresent()) {
        // Uses the primary or secondary highlight?
        const auto& interpolatedHighlightTextColor = isPrimaryHighlight.getValue() ?
                                                     cellColors->linePrimaryInterpolatedHighlightTextColor : cellColors->lineSecondaryInterpolatedHighlightTextColor;
        letterColor = letterColor.interpolatedWith(interpolatedHighlightTextColor, textColorInterpolation);
    }
    // Interpolates again with the possible cursor *line*.
    if (isCursorOnLine) {
        letterColor = letterColor.interpolatedWith(cellColors->cursorLineInterpolatedHighlightTextColor, textColorInterpolation);
    }

    return letterColor;
}

int AbstractCellView::getBaseLine() const noexcept
{
    return getHeight() - 3;     // A bit up, else letters like "g" are cut.
}

int AbstractCellView::getViewRank() const noexcept
{
    return viewRank;
}

int AbstractCellView::getLetterIndexFromX(const int x) const noexcept
{
    jassert(x >= 0);            // X is negative? Strange.

    return (x - borderWidth) / getWidth1Char();
}

}   // namespace arkostracker
