#pragma once

#include "LineTrackView.h"

#include "../../cells/simpleCell/SimpleCellView.h"
#include "LineHeader.h"

namespace arkostracker 
{

/** Implementation of a LineTrackView. */
class LineTrackViewImpl final : public LineTrackView,
                                public LineHeader::Listener,
                                public SimpleCellView::Listener
{
public:
    /**
     * Constructor.
     * @param listener the listener of the events on this View.
     * @param line the "middle" line.
     * @param trackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     * @param displayedData the data to display.
     * @param normalFont the Font to use.
     * @param doubleHeightFont the Font to use for the middle line.
     */
    LineTrackViewImpl(LineTrackView::Listener& listener, int line, std::shared_ptr<TrackViewMetadata> trackViewMetadata,
                      const LineTrackView::DisplayedData& displayedData, juce::Font& normalFont, juce::Font& doubleHeightFont) noexcept;

    // LineTrack method implementations.
    // ====================================
    int calculateTrackWidth(bool minimized) const noexcept override;
    void setDisplayedData(const DisplayedData& displayedData) noexcept override;
    TrackType getTrackType() const noexcept override;
protected:
    void onResizedAccepted() noexcept override;
    juce::Font& getNormalFont() noexcept override;
    juce::Font& getDoubleHeightFont() noexcept override;
    bool isMinimized() const noexcept override;
    std::vector<AbstractCellView*> getCellViews() const noexcept override;
    void resizeCellViewCount(size_t newCount) noexcept override;
    int getCellCount() const noexcept override;
    AbstractCellView& createAndStorePrototypeCell() noexcept override;
    void refreshCell(int cellIndex, int cellRank, bool somethingToShow, bool isMiddleLine, OptionalBool primaryHighlight, bool isPlayedLine) noexcept override;
    int getTrackHeight() const noexcept override;
    void buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& lookAndFeel) noexcept override;
    OptionalValue<CursorLocation> applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept override;
    OptionalInt getPlayedLocationLine() const noexcept override;

public:

    // LineHeader::Listener method implementations.
    // ===============================================
    void onUserClickedOnHeight() override;
    void onUserUsedMouseWheelOnHeight(bool up, bool isShiftPressed) override;

    // SimpleCellView::Listener method implementations.
    // ===================================================
    void onUserClickedOnRankOfSpecialTrack(int viewRank, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromSpecialTrack(bool leftButton) override;
    void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) override;

    // Component method implementations.
    // ======================================
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static const int digitCountIfHexadecimal;
    static const int digitCountIfDecimal;

    /**
     * Finds and sets the text color, from the given Look And Feel.
     * @param lookAndFeel the look and feel.
     */
    void findAndSetTextColor(const juce::LookAndFeel& lookAndFeel) noexcept;

    LineTrackView::Listener& listener;

    LineTrackView::DisplayedData displayedData;

    juce::Font& normalFont;
    juce::Font& doubleHeightFont;

    LineHeader lineHeader;
    juce::Colour textColor;

    std::vector<std::unique_ptr<SimpleCellView>> cells;                     // The visible Cells.

    SimpleCellView::DisplayedData outOfBoundsDisplayedData;                 // Cached DisplayedData when out of bounds.
};

}   // namespace arkostracker
