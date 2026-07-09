#include "AbstractTrackView.h"

#include <BinaryData.h>

#include "../../../../song/tracks/TrackConstants.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

AbstractTrackView::AbstractTrackView(const int pLine, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata) noexcept :
        shownLine(pLine),
        currentCursorLocation(),
        trackViewMetadata(std::move(pTrackViewMetadata)),
        trackColors(),              // Set below.
        borderColor(),              // Idem.
        lockImage(juce::ImageFileFormat::loadFrom(BinaryData::Lock_png, BinaryData::Lock_pngSize))
{
    // Sets up the Track/Cell colors.
    buildTrackColorsFromLookAndFeelInternal();
}

void AbstractTrackView::setShownLineAndCursorPositionAndRefreshIfDifferent(const int newLine, const OptionalValue<CursorLocation> newCursorLocation) noexcept
{
    // Does this Track can handle the cursor where it is? If not, no need to bother with it.
    const auto localCursorLocation = applyCursorLocationOnTrack(newCursorLocation);

    // Any change?
    if ((shownLine == newLine) && (localCursorLocation == currentCursorLocation)) {
        return;
    }

    shownLine = newLine;
    currentCursorLocation = localCursorLocation;

    refreshCells();
}

int AbstractTrackView::getFirstCellY() noexcept
{
    return headerHeight;
}

int AbstractTrackView::determineRankFromCenterAsOrigin(const int viewRank) const noexcept
{
    jassert(viewRank >= 0);
    const auto cellCount = getCellCount();
    return viewRank - calculateMiddleCellRank(cellCount);
}

int AbstractTrackView::getRankOfFirstCell() const noexcept
{
    const auto cellViews = getCellViews();
    if (cellViews.empty()) {
        jassertfalse;
        return 0;
    }
    return cellViews.at(0U)->getViewRank();
}

int AbstractTrackView::getRankOfLastCell() const noexcept
{
    const auto cellViews = getCellViews();
    if (cellViews.empty()) {
        jassertfalse;
        return 0;
    }
    return cellViews.crbegin().operator*()->getViewRank();       // Takes the last one.
}

void AbstractTrackView::resized()
{
    generateAndLocateCells();
}

void AbstractTrackView::generateAndLocateCells(const bool forceLocating) noexcept
{
    const auto totalHeight = getHeight();

    // Sanity check to prevent strange thing from happening in corner cases.
    if (totalHeight <= 0) {
        return;
    }

    const auto cellFirstY = getFirstCellY();
    const auto& normalFont = getNormalFont();
    const auto& doubleHeightFont = getDoubleHeightFont();

    const auto mustZoomMiddleLine = trackViewMetadata->zoomMiddleLine;

    // Calculates from the height, takes care of one middle (which may be doubled or not). +1 for the one in the middle, which is subtracted from the height.
    const auto calculatedCellCountFloat = ((static_cast<float>(totalHeight) - static_cast<float>(cellFirstY)
                                          - (mustZoomMiddleLine ? doubleHeightFont.getHeight() : normalFont.getHeight()))
                                          / normalFont.getHeight()) + 1.0F;
    if (calculatedCellCountFloat <= 1.0F) {
        // Another corner-case, on creating the arrangement, very small sizes can be calculated at first, and strange cell count can emerge. Ignore these.
        return;
    }

    onResizedAccepted();

    const auto expectedCellCount = static_cast<size_t>(std::ceil(calculatedCellCountFloat));

    const auto cellWidth = calculateTrackWidth(isMinimized());

    const auto cells = getCellViews();

    auto currentCellCount = cells.size();
    // Nothing to do? Then exits.
    if (currentCellCount == expectedCellCount) {
        // However, don't forget to apply the width, as it can have changed.
        for (const auto& cell : cells) {
            cell->setBounds(cell->getX(), cell->getY(), cellWidth, cell->getHeight());
        }

        if (!forceLocating) {
            return;
        }
    }

    const auto middleIndex = calculateMiddleCellRank(static_cast<int>(expectedCellCount));

    // Creates only empty Cells for now. They are updated at the end.
    if (currentCellCount < expectedCellCount) {
        // Must create new Cells.
        while (currentCellCount < expectedCellCount) {
            auto& cell = createAndStorePrototypeCell();

            addAndMakeVisible(cell);           // Location is not set right now, but below.

            ++currentCellCount;
        }
    } else if (currentCellCount > expectedCellCount) {
        // Deletes the exceeding Cells.
        resizeCellViewCount(expectedCellCount);     // Cannot modify Cells directly, this is only a copy, it wouldn't do anything!
    }

    // Sets the bounds for all the cells, as the middle cell has probably changed.
    // The font size is also set.
    auto y = cellFirstY;
    auto cellIndex = 0;
    for (const auto& cell : getCellViews()) {         // Important to get them again, their number may have changed.
        constexpr auto x = 0;
        const auto isMiddle = (cellIndex == middleIndex);
        const auto mustZoomThisLine = isMiddle && mustZoomMiddleLine;
        // If in the middle, use double-height. Why -2? Because it looks better.
        const auto cellHeight = static_cast<int>((mustZoomThisLine ? doubleHeightFont.getHeight() - 2.0F : normalFont.getHeight()));

        cell->setBounds(x, y, cellWidth, cellHeight);
        cell->setUseDoubleFontAndRepaint(mustZoomThisLine);

        y += cellHeight;
        ++cellIndex;
    }

    refreshCells();
}

void AbstractTrackView::paintOverChildren(juce::Graphics &g)
{
    const auto height = getHeight();
    const auto heightFloat = static_cast<float>(getHeight());
    const auto trackHeight = height - headerHeight;
    constexpr auto trackY = headerHeight;
    const auto width = getWidth();

    // Displays the vertical borders.
    constexpr auto borderWidth = 1.0F;

    constexpr auto borderWidthMid = borderWidth / 2.0F;
    constexpr auto leftBorderX = borderWidthMid;
    const auto rightBorderX = static_cast<float>(width) - borderWidthMid;
    g.setColour(borderColor);
    g.drawLine(leftBorderX, static_cast<float>(trackY), leftBorderX, heightFloat, borderWidth);
    g.drawLine(rightBorderX, static_cast<float>(trackY), rightBorderX, heightFloat, borderWidth);

    postPaintOverChildren(g, trackY, trackHeight);
}

void AbstractTrackView::postPaintOverChildren(juce::Graphics& /*g*/, int /*trackY*/, int /*trackHeight*/) noexcept
{
    // Nothing to do.
}

void AbstractTrackView::onLookAndFeelChanged(std::shared_ptr<TrackViewMetadata> newTrackViewMetadata) noexcept
{
    trackViewMetadata = std::move(newTrackViewMetadata);

    buildTrackColorsFromLookAndFeelInternal();

    generateAndLocateCells(true);       // In case the Zoom Middle Line changes.

    // Gives the new colors to each Cell. We don't refresh it, because we do it on the second pass below.
    for (auto* cell : getCellViews()) {
        cell->updateColors(trackColors->cellColors, false);
    }
    refreshCells();         // Required to refresh the "business" colors (effects, etc.).
}

void AbstractTrackView::buildTrackColorsFromLookAndFeelInternal() noexcept
{
    // Caches the colors from the Look and Feel.
    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    auto cellColors = CellColors(
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerBackground)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoEffect)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorBackground)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorTextHighlight)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSelection)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundPrimary)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundSecondary)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightTextPrimary)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightTextSecondary)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorLineBackground)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorLineHighlightText)),
            localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerPlayedLineBackground))
            );
    const auto errorColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerError));
    const auto noNoteColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoNote));

    trackColors = std::make_shared<TrackColors>(cellColors, errorColor, noNoteColor);

    borderColor = localLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackBorder));

    // Any work for the subclass?
    buildTrackColorsFromLookAndFeel(localLookAndFeel);
}

void AbstractTrackView::buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& /*lookAndFeel*/) noexcept
{
    // Nothing to do.
}

void AbstractTrackView::refreshCells() noexcept
{
    const auto cells = getCellViews();

    auto cellRank = 0;
    const auto size = static_cast<int>(cells.size());
    const auto middleRank = calculateMiddleCellRank(size);
    auto cellIndex = shownLine - middleRank;           // May be negative or out of bounds!
    const auto trackHeight = getTrackHeight();
    jassert(trackHeight <= TrackConstants::maximumCapacity);

    auto highlightStep = trackViewMetadata->highlightStep;
    auto secondaryHighlight = trackViewMetadata->secondaryHighlight;
    // Sanity check, a malformed song could make the code below crash.
    if ((highlightStep <= 1) || (secondaryHighlight <= 1)) {
        jassertfalse;       // Should never happen!!
        highlightStep = 4;
        secondaryHighlight = 4;
    }

    const auto playedLocationLine = getPlayedLocationLine();

    for (auto i = 0; i < size; ++i) {
        // Anything to show?
        const auto showSomething = ((cellIndex >= 0) && (cellIndex < trackHeight));

        // Highlight? Manages it only if there is something to show.
        auto primaryHighlight = OptionalBool();
        if (showSomething) {
            if ((cellIndex % (highlightStep * secondaryHighlight)) == 0) {
                primaryHighlight = OptionalBool(false);
            } else if ((cellIndex % highlightStep) == 0) {
                primaryHighlight = OptionalBool(true);
            }
        }

        const auto isMiddleRank = (middleRank == cellRank);
        const auto showPlayLine = playedLocationLine.isPresent() && (playedLocationLine.getValue() == cellIndex);

        refreshCell(cellIndex, cellRank, showSomething, isMiddleRank, primaryHighlight, showPlayLine);

        ++cellRank;
        ++cellIndex;
    }
}

void AbstractTrackView::displayReadOnlyLock(juce::Graphics& g, const bool readOnly) const noexcept
{
    if (!readOnly) {
        return;
    }

    displayOverlayImage(g, lockImage, true);
}

void AbstractTrackView::displayOverlayImage(juce::Graphics& g, const juce::Image& image, const bool onTop) const noexcept
{
    g.setColour(juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId).withAlpha(0.4F));

    const auto width = getWidth();
    const auto margins = LookAndFeelConstants::margins;

    const auto originalLockWidth = image.getWidth();
    const auto originalLockHeight = image.getHeight();
    auto lockWidth = originalLockWidth;
    auto lockHeight = originalLockHeight;
    if (lockWidth > width) {
        const auto imageRatio = static_cast<double>(originalLockWidth) / originalLockHeight;
        lockWidth = width - LookAndFeelConstants::margins;
        lockHeight = static_cast<int>(static_cast<double>(lockWidth) * imageRatio);
    }

    const auto lockX = (width - lockWidth) / 2;
    const auto lockY = onTop ? headerHeight + margins : getHeight() - margins - lockHeight;
    g.drawImage(image, lockX, lockY, lockWidth, lockHeight,
        0, 0, originalLockWidth, originalLockHeight, true);
}

}   // namespace arkostracker
