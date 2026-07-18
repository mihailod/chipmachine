#pragma once

#include "../AbstractTrackView.h"

#include "../../../../../song/tracks/SpecialTrack.h"
#include "../../../controller/Selection.h"
#include "../../cells/simpleCell/SimpleCellView.h"
#include "SpecialTrackHeader.h"

namespace arkostracker 
{

/** Abstract class of a Special TrackView. */
class SpecialTrackView : public AbstractTrackView,
                         public SimpleCellView::Listener
{
public:
    /** Listener to the events of this Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user clicked on a specific rank of the Cell.
         * @param isSpeedTrack true is Speed Track, false if Event Track.
         * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
         * @param cursorRank the rank.
         * @param leftButton true if left Button, false if right.
         * @param shift true if shift is pressed.
         */
        virtual void onUserClickedOnRankOfSpecialTrack(bool isSpeedTrack, int rankFromCenterAsOrigin, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) = 0;

        /**
         * Called when the user click is now up. Only called after a down has been clicked.
         * @param leftButton true if left Button, false if right.
         */
        virtual void onUserClickIsUpFromSpecialTrack(bool leftButton) = 0;

        /**
         * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
         * It is up to the client to find it.
         * @param event the mouse event.
         */
        virtual void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) = 0;

        /**
         * The users wants the name the Special Track.
         * @param isSpeedTrack true if Speed Track, false if Event.
         * @param currentName the current name for display. This is a simplification to bypass the need to ask for it to the controller.
         */
        virtual void onUserWantsToNameSpecialTrack(bool isSpeedTrack, const juce::String& currentName) = 0;

        /**
         * The user wants to link/unlink.
         * @param isSpeedTrack true if Speed Track, false if Event.
         */
        virtual void onUserWantsToManageSpecialLink(bool isSpeedTrack) = 0;
    };

    /** The data to display. Must not be stored. */
    class TransmittedDisplayedData
    {
    public:
        /**
         * Constructor.
         * @param pLine the line.
         * @param pHeight the track height (>0).
         * @param pSpecialTrack the Special Track, present only if changed. Must be copied!
         * @param pPlayedLocationLine the played location line, if any.
         * @param pSelection the selection on THIS track. If empty, nothing to show.
         * @param pLinkState the link state to show.
         */
        TransmittedDisplayedData(const int pLine, const int pHeight, const SpecialTrack* pSpecialTrack, const OptionalInt pPlayedLocationLine, const Selection& pSelection,
                                 const LinkState pLinkState) :
                line(pLine),
                height(pHeight),
                specialTrack(pSpecialTrack),
                playedLocationLine(pPlayedLocationLine),
                selection(pSelection),
                linkState(pLinkState)
        {
        }
        /** Copy forbidden. */
        TransmittedDisplayedData(const TransmittedDisplayedData&) = delete;

        const int line;
        const int height;
        const SpecialTrack* specialTrack;               // Present if not changed.
        const OptionalInt playedLocationLine;
        const Selection selection;
        const LinkState linkState;
    };

    /**
     * Constructor.
     * @param listener the listener to the event of this Component.
     * @param line the "middle" line to show.
     * @param height the height of the Track (>0).
     * @param trackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     * @param normalFont the Font to use.
     * @param doubleHeightFont the Font to use for the middle line.
     */
    SpecialTrackView(Listener& listener, int line, int height, std::shared_ptr<TrackViewMetadata> trackViewMetadata, juce::Font& normalFont, juce::Font& doubleHeightFont) noexcept;

    /**
     * Sets the data to display. This may refresh the UI if needed.
     * @param transmittedDisplayedData the data to display. The track inside is not present if not changed. Do not store this object!
     */
    void setDisplayedDataAndRefreshIfNeeded(const TransmittedDisplayedData& transmittedDisplayedData) noexcept;

    int calculateTrackWidth(bool minimized) const noexcept override;

    // SimpleCellView::Listener method implementations.
    // ===================================================
    void onUserClickedOnRankOfSpecialTrack(int viewRank, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUpFromSpecialTrack(bool leftButton) override;
    void onUserDraggedCursorFromSpecialTrack(const juce::MouseEvent& event) override;

protected:
    juce::Font& getNormalFont() noexcept override;
    juce::Font& getDoubleHeightFont() noexcept override;
    bool isMinimized() const noexcept override;
    void refreshCell(int cellIndex, int cellRank, bool somethingToShow, bool isMiddleLine, OptionalBool primaryHighlight, bool isPlayedLine) noexcept override;
    std::vector<AbstractCellView*> getCellViews() const noexcept override;
    void resizeCellViewCount(size_t newCount) noexcept override;
    int getCellCount() const noexcept override;
    AbstractCellView& createAndStorePrototypeCell() noexcept override;
    int getTrackHeight() const noexcept override;
    OptionalValue<CursorLocation> applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept override;
    OptionalInt getPlayedLocationLine() const noexcept override;
    void postPaintOverChildren(juce::Graphics& g, int trackY, int trackHeight) noexcept override;

    /** @return the base color of the text. */
    virtual juce::Colour getCellTextColor() const noexcept = 0;

    /** @return true if Speed Track, false if Event Track. */
    virtual bool isSpeedTrack() const noexcept = 0;

    /**
     * Sets the data to display for the header. Nothing happens if there is no change.
     * @param displayedData the displayed data for the header.
     */
    virtual void setHeaderDisplayedData(const SpecialTrackHeader::DisplayedData& displayedData) noexcept = 0;

    Listener& listener;
    int trackHeight;                                                        // The track height.
    juce::Font& normalFont;
    juce::Font& doubleHeightFont;
    LinkState linkState;

    SpecialTrack specialTrack;                                              // The displayed data.
    OptionalInt playedLocationLine;

    std::vector<std::unique_ptr<SimpleCellView>> cells;                     // The visible Cells.
    Selection selection;                                                    // A possible selection.

    SimpleCellView::DisplayedData outOfBoundsDisplayedData;                 // Cached DisplayedData when out of bounds.
};

}   // namespace arkostracker
