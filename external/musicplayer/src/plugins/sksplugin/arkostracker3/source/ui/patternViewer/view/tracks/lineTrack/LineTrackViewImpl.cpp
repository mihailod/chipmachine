#include "LineTrackViewImpl.h"

#include "../../../../../utils/CollectionUtil.h"
#include "../../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int LineTrackViewImpl::digitCountIfHexadecimal = 2;
const int LineTrackViewImpl::digitCountIfDecimal = 3;

LineTrackViewImpl::LineTrackViewImpl(LineTrackView::Listener& pListener, const int pLine, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata,
                                     const DisplayedData& pDisplayedData, juce::Font& pNormalFont, juce::Font& pDoubleHeightFont) noexcept :
        LineTrackView(pLine, std::move(pTrackViewMetadata)),
        listener(pListener),
        displayedData(pDisplayedData),
        normalFont(pNormalFont),
        doubleHeightFont(pDoubleHeightFont),
        lineHeader(*this, displayedData.getHeight(), pDisplayedData.isHexadecimal()),
        textColor(juce::Colours::red),     // Only temporary.
        cells(),
        outOfBoundsDisplayedData()
{
    findAndSetTextColor(juce::LookAndFeel::getDefaultLookAndFeel());

    addAndMakeVisible(lineHeader);
}


// LineTrack method implementations.
// ====================================

void LineTrackViewImpl::setDisplayedData(const DisplayedData& newDisplayedData) noexcept
{
    // Any change?
    if ((displayedData == newDisplayedData) && (newDisplayedData.getLine() == shownLine)) { // Second comparison is a bit hackish, see below.
        return;
    }
    displayedData = newDisplayedData;
    // A bit hackish, but since the view is simple... The DisplayedData should be "transmitted" and not be stored.
    // There should be distinct object, with DisplayedData not having the line.
    shownLine = newDisplayedData.getLine();

    // The header may need refresh.
    lineHeader.setDisplayedData(displayedData.getHeight(), displayedData.isHexadecimal());

    refreshCells();
}

int LineTrackViewImpl::calculateTrackWidth(bool /*minimized*/) const noexcept
{
    const auto charWidth = AbstractCellView::calculateCharWidth(normalFont);
    const auto digitCount = displayedData.isHexadecimal() ? digitCountIfHexadecimal : digitCountIfDecimal;
    // The side border + the chars.
    return (AbstractCellView::borderWidth * 2) + (digitCount * charWidth);
}

TrackType LineTrackViewImpl::getTrackType() const noexcept
{
    return TrackType::line;
}

void LineTrackViewImpl::onResizedAccepted() noexcept
{
    // Sets the header new size.
    lineHeader.setBounds(0, 0, getWidth(), headerHeight);
}

juce::Font& LineTrackViewImpl::getNormalFont() noexcept
{
    return normalFont;
}

juce::Font& LineTrackViewImpl::getDoubleHeightFont() noexcept
{
    return doubleHeightFont;
}

bool LineTrackViewImpl::isMinimized() const noexcept
{
    return false;
}

std::vector<AbstractCellView*> LineTrackViewImpl::getCellViews() const noexcept
{
    return CollectionUtil::toPointerCollection<SimpleCellView, AbstractCellView>(cells);
}

void LineTrackViewImpl::resizeCellViewCount(const size_t newCount) noexcept
{
    jassert(newCount <= cells.size());
    cells.resize(newCount);
}

int LineTrackViewImpl::getCellCount() const noexcept
{
    return static_cast<int>(cells.size());
}

AbstractCellView& LineTrackViewImpl::createAndStorePrototypeCell() noexcept
{
    const auto cellDisplayedData = SimpleCellView::DisplayedData();
    auto cell = std::make_unique<SimpleCellView>(*this, static_cast<int>(cells.size()), normalFont, doubleHeightFont, cellDisplayedData, trackColors->cellColors,
            digitCountIfHexadecimal, digitCountIfDecimal, SimpleCellView::DisplayIfZero::showZeros);
    cells.push_back(std::move(cell));

    // Returns the last one.
    return cells.rbegin()->operator*();
}

void LineTrackViewImpl::buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& paramLookAndFeel) noexcept
{
    findAndSetTextColor(paramLookAndFeel);
}

OptionalValue<CursorLocation> LineTrackViewImpl::applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& /*cursorLocationToTest*/) const noexcept
{
    // There is no cursor on a Line Track.
    return { };
}

void LineTrackViewImpl::findAndSetTextColor(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    textColor = pLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerLineTrackText));
}

void LineTrackViewImpl::refreshCell(const int cellIndex, const int cellRank, const bool somethingToShow, const bool isMiddleLine,
                                    const OptionalBool primaryHighlight, const bool isPlayedLine) noexcept
{
    const auto& cellView = cells.at(static_cast<size_t>(cellRank));

    if (!somethingToShow) {
        // Nothing to show.
        cellView->updateDisplayedData(outOfBoundsDisplayedData);
        return;
    }

    // Something to show.
    constexpr auto cursorRank = SpecialCellCursorRank::notPresent;        // No cursor on the Line Track.
    const auto isSelected = displayedData.getSelection().containsLine(cellIndex);
    const SimpleCellView::DisplayedData cellDisplayedData(cursorRank, isMiddleLine, primaryHighlight, isPlayedLine,
                                                    cellIndex, displayedData.isHexadecimal(), textColor, isSelected);

    cellView->updateDisplayedData(cellDisplayedData);
}

int LineTrackViewImpl::getTrackHeight() const noexcept
{
    return displayedData.getHeight();
}


// LineHeader::Listener method implementations.
// ===============================================

void LineTrackViewImpl::onUserClickedOnHeight()
{
    listener.onUserClickedOnTrackHeight();
}

void LineTrackViewImpl::onUserUsedMouseWheelOnHeight(const bool up, const bool isShiftPressed)
{
    const auto step = isShiftPressed ? 2 : 1;
    const auto offset = up ? step : -step;
    listener.onUserWantsToModifyTrackHeightWithOffset(offset);
}

OptionalInt LineTrackViewImpl::getPlayedLocationLine() const noexcept
{
    return displayedData.getPlayedLocationLine();
}


// SimpleCellView::Listener method implementations.
// ===================================================

void LineTrackViewImpl::onUserClickedOnRankOfSpecialTrack(const int viewRank, SpecialCellCursorRank /*cursorRank*/, const bool leftButton, bool /*shift*/)
{
    // The controller does not know where is the centered rank. Makes sure the center in 0.
    const auto rankFromCenterAsOrigin = determineRankFromCenterAsOrigin(viewRank);
    listener.onUserClickedOnLineTrack(rankFromCenterAsOrigin, leftButton);
}

void LineTrackViewImpl::onUserClickIsUpFromSpecialTrack(const bool leftButton)
{
    listener.onUserClickIsUpFromLineTrack(leftButton);
}

void LineTrackViewImpl::onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event)
{
    listener.onUserDraggedCursorFromLineTrack(event);
}

// Component method implementations.
// ======================================
void LineTrackViewImpl::mouseWheelMove(const juce::MouseEvent& /*event*/, const juce::MouseWheelDetails& wheel)
{
    if (!juce::exactlyEqual(wheel.deltaY, 0.0F)) {
        listener.onUserWantsToScrollVerticallyWithMouseWheelOnLineTrack(wheel.deltaY < 0.0F);
    }
}

}   // namespace arkostracker
