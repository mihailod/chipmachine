#include "SimpleCellView.h"

#include "../../../../../utils/NumberUtil.h"

namespace arkostracker 
{

const juce::String SimpleCellView::twoDashes = "--";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

SimpleCellView::SimpleCellView(Listener& pListener, const int pCellRank, const juce::Font& pFont, const juce::Font& pDoubleFont, const DisplayedData& pDisplayedData,
                               const CellColors& pCellColors, int /*pDigitCountIfHexadecimal*/, const int pDigitCountIfDecimal,
                               const DisplayIfZero pDisplayIfZero) noexcept :
        AbstractCellView(pCellRank, pFont, pDoubleFont, pCellColors),
        listener(pListener),
        displayedData(pDisplayedData),
        //digitCountIfHexadecimal(pDigitCountIfHexadecimal),
        digitCountIfDecimal(pDigitCountIfDecimal),
        displayIfZero(pDisplayIfZero)
{
}

int SimpleCellView::calculateWidth(const juce::Font& pFont) noexcept
{
    const auto widthChar = calculateCharWidth(pFont);
    return (widthChar * 2) + (borderWidth * 2);
}

void SimpleCellView::setUseDoubleFontAndRepaint(const bool newUseDoubleHeightFont) noexcept
{
    if (useDoubleFont != newUseDoubleHeightFont) {
        useDoubleFont = newUseDoubleHeightFont;
        repaint();
    }
}

int SimpleCellView::getCellRankFromX(const int x) const noexcept
{
    const auto rank = (x < (borderWidth + getWidth1Char())) ? SpecialCellCursorRank::cursorOnDigit2 : SpecialCellCursorRank::cursorOnDigit1;
    return static_cast<int>(rank);
}

void SimpleCellView::updateDisplayedData(const SimpleCellView::DisplayedData& newDisplayedData) noexcept
{
    // Any change? If not, exits.
    if (displayedData == newDisplayedData) {
        return;
    }

    displayedData = newDisplayedData;

    repaint();
}


// Component method implementations.
// =====================================

void SimpleCellView::paint(juce::Graphics& g)
{
    // Fills the background.
    const auto isCursorOnLine = displayedData.isCursorOnLine();
    const auto& primaryHighlight = displayedData.getPrimaryHighlight();
    const auto currentBackgroundColor = determineBackgroundColor(displayedData.isShowSomething(), primaryHighlight,
                                                                 isCursorOnLine, displayedData.isPlayedLine());
    g.fillAll(currentBackgroundColor);

    if (!displayedData.isShowSomething()) {
        return;
    }

    // Shows the number.
    g.setFont(useDoubleFont ? doubleFont : font);
    const auto value = displayedData.getValue();

    const auto& textBaseColor = displayedData.getTextColor();

    // The text can be dashes only, if the value is 0, if wanted.
    const auto isHexadecimal = displayedData.isHexadecimal();
    const auto showValueWithNumber = ((value != 0) || (displayIfZero == DisplayIfZero::showZeros));
    const auto text = showValueWithNumber ?
        (isHexadecimal ? NumberUtil::toHexByte(value) : NumberUtil::toDecimalString(value, digitCountIfDecimal)) :
        // Show dashes or nothing.
        (displayIfZero == DisplayIfZero::dontShow) ? juce::String() : twoDashes; // We consider this is used only for two-digits columns. This is unlikely to change!

    const auto height = getHeight();
    const auto baselineY = getBaseLine();

    const auto cursorRank = displayedData.getCursorRank();
    const auto isCursorOnDigit2 = (cursorRank == SpecialCellCursorRank::cursorOnDigit2);
    const auto isCursorOnDigit1 = (cursorRank == SpecialCellCursorRank::cursorOnDigit1);

    auto letterIndex = 0;
    // The digit3 is only for the Decimal (only used in the Line Track), there is no cursor for it.
    if (!isHexadecimal) {
        displayChar(g, letterIndex, baselineY, height, text.substring(letterIndex, letterIndex + 1), textBaseColor, false,
                    isCursorOnLine, primaryHighlight);
        ++letterIndex;
    }
    // Digit 2.
    displayChar(g, letterIndex, baselineY, height, text.substring(letterIndex, letterIndex + 1), textBaseColor, isCursorOnDigit2,
                isCursorOnLine, primaryHighlight);
    // Digit 1.
    ++letterIndex;
    displayChar(g, letterIndex, baselineY, height, text.substring(letterIndex, letterIndex + 1), textBaseColor, isCursorOnDigit1,
                isCursorOnLine, primaryHighlight);

    // Draws the selection, if any.
    if (displayedData.isSelected()) {
        const auto selectionWidth = getWidth1Char() * 2;

        g.setColour(cellColors->selectionColor.withAlpha(selectionAlpha));
        g.fillRect(borderWidth, 0, selectionWidth, height);
    }
}

void SimpleCellView::mouseDown(const juce::MouseEvent& event)
{
    const auto clickedX = event.getPosition().getX();
    const auto letterIndex = getLetterIndexFromX(clickedX);
    const auto rank = (letterIndex == 0) ? SpecialCellCursorRank::cursorOnDigit2 : SpecialCellCursorRank::cursorOnDigit1;

    const auto leftButton = event.mods.isLeftButtonDown();
    const auto shift = event.mods.isShiftDown();
    listener.onUserClickedOnRankOfSpecialTrack(viewRank, rank, leftButton, shift);
}

void SimpleCellView::mouseDrag(const juce::MouseEvent& event)
{
    listener.onUserDraggedCursorFromSpecialTrack(event);
}

void SimpleCellView::mouseUp(const juce::MouseEvent& event)
{
    const auto leftButton = event.mods.isLeftButtonDown();
    listener.onUserClickIsUpFromSpecialTrack(leftButton);
}

}   // namespace arkostracker
