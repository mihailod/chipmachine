#pragma once

#include <vector>

#include "../../CursorLocation.h"
#include "../cells/AbstractCellView.h"
#include "TrackType.h"
#include "TrackViewMetadata.h"
#include "track/TrackColors.h"

namespace arkostracker 
{

/** An abstract Track View (for "music" Track, and speed/event Track). */
class AbstractTrackView : public juce::Component
{
public:
    static constexpr auto headerHeight = 26;

    /** Constructor. */
    AbstractTrackView(int line, std::shared_ptr<TrackViewMetadata> trackViewMetadata) noexcept;

    /**
     * Sets the index to show "in the middle", and the cursor is wanted. This provokes a repaint, but only if the index or cursor has changed.
     * @param line the line index. Should be positive and valid, but the View should handle wrong values.
     * @param cursorLocation the possible cursor, if its position changed. Note that the position is generic and thus, may be out of this Track!
     */
    void setShownLineAndCursorPositionAndRefreshIfDifferent(int line, OptionalValue<CursorLocation> cursorLocation) noexcept;

    /**
     * @return the width of a track.
     * @param minimized true of minimized. May not be relevant, in which case it can be ignored.
     */
    virtual int calculateTrackWidth(bool minimized) const noexcept = 0;

    /**
     * Called by the parent view when the look and feel changed, because it needs the TrackViewMetadata to change all the colors.
     * The client should only store the
     * @param trackViewMetadata the new metadata to use.
     */
    void onLookAndFeelChanged(std::shared_ptr<TrackViewMetadata> trackViewMetadata) noexcept;

    /** @return the type of Track. */
    virtual TrackType getTrackType() const noexcept = 0;

    /**
     * @return the rank with the center as origin, from the given rank.
     * @param viewRank the rank (>=0). This is NOT the cell index! This is the rank from the top of the displayed Cells.
     */
    int determineRankFromCenterAsOrigin(int viewRank) const noexcept;

    /** @return the View Rank of the first cell. */
    int getRankOfFirstCell() const noexcept;
    /** @return the View Rank of the last cell. */
    int getRankOfLastCell() const noexcept;

    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;

protected:
    /**
     * @return the rank of the highlighted Cell "in the middle".
     * @param cellCount how many Cells there are (even if not fully visible).
     */
    static int calculateMiddleCellRank(const int cellCount) noexcept
    {
        return static_cast<int>(static_cast<float>(cellCount) / 2.1F);       // Slightly above the middle. Works fine!
    }

    /**
     * Allows the children to draw something after the parent did something on the paintOverChildren.
     * The default implementation does nothing.
     * @param g the Graphic object.
     * @param trackY Y of the track (without the header), as a convenience.
     * @param trackHeight the height of the track (without the header), as a convenience.
     */
    virtual void postPaintOverChildren(juce::Graphics& g, int trackY, int trackHeight) noexcept;

    /** Called when the resized has valid width/height. The implementation can, for example, set the bounds of the header. */
    virtual void onResizedAccepted() noexcept = 0;

    /** @return the Y of the first cell (for example, just below a header). */
    virtual int getFirstCellY() noexcept;

    /** @return the "normal" font to use. */
    virtual juce::Font& getNormalFont() noexcept = 0;
    /** @return the "double height normal" font to use. */
    virtual juce::Font& getDoubleHeightFont() noexcept = 0;

    /** @return true if the track is minimized. */
    virtual bool isMinimized() const noexcept = 0;

    /** @return the CellViews. */
    virtual std::vector<AbstractCellView*> getCellViews() const noexcept = 0;
    /**
     * Sets how many Views there must be. This MUST be a smaller count than what is already existing!
     * @param newCount the new count.
     */
    virtual void resizeCellViewCount(size_t newCount) noexcept = 0;

    /** @return how many cells are instantiated. */
    virtual int getCellCount() const noexcept = 0;

    /**
     * @return a reference to a new instance of a "prototype" Cell that the client must instantiate and store at the end of its Cell list.
     * The Cell can be empty, it will be filled later.
     */
    virtual AbstractCellView& createAndStorePrototypeCell() noexcept = 0;

    /**
     * Creates as many cells as necessary (or remove them), and locates them vertically.
     * @param forceLocating true to force the positioning in Y. Useful when changing Zoom Middle Line.
     */
    void generateAndLocateCells(bool forceLocating = false) noexcept;

    /**
     * Refreshes the data of all the Cells, from the internal Track, updating the UI, if necessary.
     * The algorithm will call the refreshCell methods.
     */
    void refreshCells() noexcept;

    /**
     * Displays the read-only image, if the track is read-only.
     * @param g the Graphics.
     * @param readOnly true if read-only.
     */
    void displayReadOnlyLock(juce::Graphics& g, bool readOnly) const noexcept;

    /**
     * Refreshes the data of one Cell, from the internal data, updating the UI, if necessary.
     * @param cellIndex the cell index, in the Track. Is "somethingToShow" is true, this value is valid.
     * @param cellRank the rank of the Cell, inside the stored Cell list.
     * @param somethingToShow true of there should be something displayed, false if not (out of bounds).
     * @param isMiddleLine true if the Cell is in "the middle", thus highlighted.
     * @param primaryHighlight if present, true if primary highlight, false if secondary.
     * @param isPlayedLine true if this line is being played.
     */
    virtual void refreshCell(int cellIndex, int cellRank, bool somethingToShow, bool isMiddleLine, OptionalBool primaryHighlight,
                             bool isPlayedLine) noexcept = 0;

    /** @return the height of the Track (>0). */
    virtual int getTrackHeight() const noexcept = 0;

    /**
     * Called when the look and feel changed. The subclasses may want to store an inner value they are interested in.
     * No need to refresh the Cells.
     * The default implementation does nothing.
     * @param lookAndFeel the current look and feel. Do not store it.
     */
    virtual void buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& lookAndFeel) noexcept;

    /**
     * Tries to apply the given CursorLocation to the current Track. If the cursor cannot be put on this Track, empty is returned.
     * @param cursorLocationToTest the cursor location to test.
     * @return the cursor, or empty.
     */
    virtual OptionalValue<CursorLocation> applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept = 0;

    /** @returns the played location line. If absent, it must not be shown (follow on for example, or the played location is not on this position). */
    virtual OptionalInt getPlayedLocationLine() const noexcept = 0;

    int shownLine;
    OptionalValue<CursorLocation> currentCursorLocation;                    // The possible cursor. It must be valid for this Track.

    std::shared_ptr<TrackViewMetadata> trackViewMetadata;                   // View info (besides the Track) to display all this properly.
    std::shared_ptr<TrackColors> trackColors;                               // Will be updated on look'n'feel change.
    juce::Colour borderColor;                                               // The color of the vertical borders.

    juce::Image lockImage;

private:
    /** Builds the internal TrackColors object from the Look and Feel. This will call the protected method. */
    void buildTrackColorsFromLookAndFeelInternal() noexcept;

    /**
     * Displays an overlay image (link, lock).
     * @param g the Graphics.
     * @param image the image.
     * @param onTop true to show on top, false to show on bottom.
     */
    void displayOverlayImage(juce::Graphics& g, const juce::Image& image, bool onTop) const noexcept;
};

}   // namespace arkostracker
