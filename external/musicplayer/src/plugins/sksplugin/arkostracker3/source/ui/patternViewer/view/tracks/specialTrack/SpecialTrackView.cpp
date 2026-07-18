#include "SpecialTrackView.h"

#include "../../../../../utils/CollectionUtil.h"

namespace arkostracker 
{

SpecialTrackView::SpecialTrackView(Listener& pListener, const int pLine, const int pHeight, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata,
                                   juce::Font& pNormalFont, juce::Font& pDoubleHeightFont) noexcept :
        AbstractTrackView(pLine, std::move(pTrackViewMetadata)),
        listener(pListener),
        trackHeight(pHeight),
        normalFont(pNormalFont),
        doubleHeightFont(pDoubleHeightFont),
        linkState(LinkState::none),
        specialTrack(),
        playedLocationLine(),
        cells(),
        selection(),
        outOfBoundsDisplayedData()
{
}

void SpecialTrackView::setDisplayedDataAndRefreshIfNeeded(const SpecialTrackView::TransmittedDisplayedData& transmittedDisplayedData) noexcept
{
    auto mustRefresh = false;

    // Line changed?
    const auto newLine = transmittedDisplayedData.line;
    if (shownLine != newLine) {
        shownLine = newLine;
        mustRefresh = true;
    }

    // Height changed?
    const auto newHeight = transmittedDisplayedData.height;
    if (trackHeight != newHeight) {
        trackHeight = newHeight;
        mustRefresh = true;
    }

    // Track changed?
    const auto& newSpecialTrack = transmittedDisplayedData.specialTrack;
    if (newSpecialTrack != nullptr) {
        specialTrack = *newSpecialTrack;
        mustRefresh = true;

        // Refreshes the header of the Special Track.
        setHeaderDisplayedData(SpecialTrackHeader::DisplayedData(specialTrack.getName(), transmittedDisplayedData.linkState));
    }
    // Link state change? It refreshes the header on its own.
    if (const auto newLinkState = transmittedDisplayedData.linkState; linkState != newLinkState) {
        linkState = newLinkState;
        // Refreshes the header of the Special Track.
        setHeaderDisplayedData(SpecialTrackHeader::DisplayedData(specialTrack.getName(), newLinkState));
    }

    // Selection changed?
    const auto& newSelection = transmittedDisplayedData.selection;
    if (selection != newSelection) {
        selection = newSelection;
        mustRefresh = true;
    }

    // Played location line changed?
    const auto newPlayedLocationLine = transmittedDisplayedData.playedLocationLine;
    if (playedLocationLine != newPlayedLocationLine) {
        playedLocationLine = newPlayedLocationLine;
        mustRefresh = true;
    }

    if (mustRefresh) {
        refreshCells();
    }
}

int SpecialTrackView::calculateTrackWidth(bool /*minimized*/) const noexcept
{
    // The side border + the chars.
    return SimpleCellView::calculateWidth(normalFont);                       // Any font will do, they have the same width.
}

juce::Font& SpecialTrackView::getNormalFont() noexcept
{
    return normalFont;
}

juce::Font& SpecialTrackView::getDoubleHeightFont() noexcept
{
    return doubleHeightFont;
}

bool SpecialTrackView::isMinimized() const noexcept
{
    return false;
}

void SpecialTrackView::refreshCell(const int cellIndex, const int cellRank, const bool somethingToShow, const bool isMiddleLine, const OptionalBool primaryHighlight, const bool isPlayedLine) noexcept
{
    const auto& cellView = cells.at(static_cast<size_t>(cellRank));

    if (!somethingToShow) {
        // Nothing to show.
        cellView->updateDisplayedData(outOfBoundsDisplayedData);
        return;
    }

    // Something to show.
    const auto value = specialTrack.getCell(cellIndex).getValue();
    const auto cursorRank = (isMiddleLine && currentCursorLocation.isPresent())
                      ? static_cast<SpecialCellCursorRank>(currentCursorLocation.getValue().getRank())
                      : SpecialCellCursorRank::notPresent;
    const auto isSelected = selection.containsLine(cellIndex);
    const SimpleCellView::DisplayedData cellDisplayedData(cursorRank, isMiddleLine, primaryHighlight, isPlayedLine, value,
                                                          true, getCellTextColor(), isSelected);

    cellView->updateDisplayedData(cellDisplayedData);
}

std::vector<AbstractCellView*> SpecialTrackView::getCellViews() const noexcept
{
    return CollectionUtil::toPointerCollection<SimpleCellView, AbstractCellView>(cells);
}

void SpecialTrackView::resizeCellViewCount(const size_t newCount) noexcept
{
    jassert(newCount <= cells.size());
    cells.resize(newCount);
}

int SpecialTrackView::getCellCount() const noexcept
{
    return static_cast<int>(cells.size());
}

AbstractCellView& SpecialTrackView::createAndStorePrototypeCell() noexcept
{
    auto cellDisplayedData = SimpleCellView::DisplayedData();
    auto cell = std::make_unique<SimpleCellView>(*this, static_cast<int>(cells.size()), normalFont, doubleHeightFont, cellDisplayedData, trackColors->cellColors,
                                                 2, 2, SimpleCellView::DisplayIfZero::dontShow);
    cells.push_back(std::move(cell));

    // Returns the last one.
    return cells.rbegin()->operator*();
}

int SpecialTrackView::getTrackHeight() const noexcept
{
    return trackHeight;
}

OptionalValue<CursorLocation> SpecialTrackView::applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept
{
    // Not present? Can't do anything then.
    if (cursorLocationToTest.isAbsent()) {
        return cursorLocationToTest;
    }

    const auto cursorLocationValueToTest = cursorLocationToTest.getValue();
    // Must be on a SpecialTrack, and there is only one "channel".
    if (cursorLocationValueToTest.getTrackType() != getTrackType() || cursorLocationValueToTest.getChannelIndex() != 0) {
        return { };
    }

    // A valid rank?
    const auto rank = cursorLocationValueToTest.getRank();
    if ((rank < 0) || (rank >= static_cast<int>(SpecialCellCursorRank::count))) {
        return { };
    }

    // Valid!
    return cursorLocationToTest;
}

OptionalInt SpecialTrackView::getPlayedLocationLine() const noexcept
{
    return playedLocationLine;
}

void SpecialTrackView::postPaintOverChildren(juce::Graphics& g, int /*trackY*/, int /*trackHeight*/) noexcept
{
    displayReadOnlyLock(g, specialTrack.isReadOnly());
}


// SimpleCellView::Listener method implementations.
// ===================================================

void SpecialTrackView::onUserClickedOnRankOfSpecialTrack(const int viewRank, const SpecialCellCursorRank cursorRank, const bool leftButton, const bool shift)
{
    // The controller does not know where is the centered rank. Makes sure the center in 0.
    const auto rankFromCenterAsOrigin = determineRankFromCenterAsOrigin(viewRank);
    listener.onUserClickedOnRankOfSpecialTrack(isSpeedTrack(), rankFromCenterAsOrigin, cursorRank, leftButton, shift);
}

void SpecialTrackView::onUserClickIsUpFromSpecialTrack(const bool leftButton)
{
    listener.onUserClickIsUpFromSpecialTrack(leftButton);
}

void SpecialTrackView::onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event)
{
    listener.onUserDraggedCursorFromSpecialTrack(event);
}

}   // namespace arkostracker
