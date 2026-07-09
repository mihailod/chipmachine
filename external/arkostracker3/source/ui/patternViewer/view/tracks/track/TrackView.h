#pragma once

#include "../../../../../song/tracks/Track.h"
#include "../../../../../utils/OptionalValue.h"
#include "../../../controller/Selection.h"
#include "../AbstractTrackView.h"
#include "../TrackViewMetadata.h"

namespace arkostracker 
{

/** A Track View (music track). It includes the header. */
class TrackView : public AbstractTrackView
{
public:
    /** Listener to the events of this Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * The user left-clicked on the channel number.
         * @param channelIndex the channel index (>=0).
         */
        virtual void onUserLeftClickedOnChannelNumber(int channelIndex) = 0;
        /**
         * The user right-clicked on the channel number.
         * @param channelIndex the channel index (>=0).
         */
        virtual void onUserRightClickedOnChannelNumber(int channelIndex) = 0;

        /**
         * The user scrolled with the mouse wheel on a "music" Track.
         * @param scrollToBottom true to go to the bottom, false to go up.
         */
        virtual void onUserWantsToScrollVerticallyWithMouseWheelOnTrack(bool scrollToBottom) = 0;

        /**
         * Called when the user clicked on a specific rank of the Cell.
         * @param channelIndex the channel of the Track (>=0).
         * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
         * @param cursorRank the rank.
         * @param leftButton true if left Button, false if right.
         * @param shift true if shift is pressed.
         */
        virtual void onUserClickedOnRankOfNormalTrack(int channelIndex, int rankFromCenterAsOrigin, CellCursorRank cursorRank, bool leftButton, bool shift) = 0;

        /**
         * Called when the user click is now up. Only called after a down has been clicked.
         * @param leftButton true if left Button, false if right.
         */
        virtual void onUserClickIsUpFromNormalTrack(bool leftButton) = 0;

        /**
         * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
         * It is up to the client to find it.
         * @param event the mouse event.
         */
        virtual void onUserDraggedCursorFromNormalTrack(const juce::MouseEvent& event) = 0;

        /**
         * The user wants the change the transposition.
         * @param channelIndex the channel index.
         */
        virtual void onUserWantsToChangeTransposition(int channelIndex) = 0;

        /**
         * The user wants the name the track.
         * @param channelIndex the channel index.
         * @param currentName the current name. This is a simplification to bypass the need to get the track name from the controller.
         * @param currentColor the possible current color. This is a simplification to bypass the need to get the track name from the controller.
         */
        virtual void onUserWantsToSetTrackNameAndColor(int channelIndex, const juce::String& currentName, OptionalValue<juce::Colour> currentColor) = 0;

        /**
         * The user wants to manage the link/unlink.
         * @param channelIndex the channel index (>=0) on which the request is performed.
         */
        virtual void onUserWantsToManageLink(int channelIndex) = 0;
    };

    /** The data received. */
    class TransmittedDisplayedData
    {
    public:
        /**
         * Constructor.
         * @param pLine the line to show.
         * @param pCursorLocation a cursor location.
         * @param pTrackHeight the height of track (>0).
         * @param pLinkState the link state.
         * @param pMuted true if the Track is muted.
         * @param pMinimized true if minimized.
         * @param pTransposition the transposition.
         * @param pTrack the track. Nullptr if not new.
         * @param pPlayedLocationLine the played location line. If not present, there must be none displayed.
         * @param pSelection the selection on THIS track. If empty, nothing to show.
         */
        TransmittedDisplayedData(const int pLine, const CursorLocation pCursorLocation, const int pTrackHeight, const LinkState pLinkState, const bool pMuted,
                const bool pMinimized, const int pTransposition, const Track* pTrack, const OptionalInt pPlayedLocationLine, const Selection& pSelection) noexcept :
                line(pLine),
                cursorLocation(pCursorLocation),
                trackHeight(pTrackHeight),
                linkState(pLinkState),
                muted(pMuted),
                minimized(pMinimized),
                transposition(pTransposition),
                track(pTrack),
                playedLocationLine(pPlayedLocationLine),
                selection(pSelection)
        {
        }

        /** Copy forbidden. */
        TransmittedDisplayedData(const TransmittedDisplayedData&) = delete;

        int line;
        CursorLocation cursorLocation;
        int trackHeight;
        LinkState linkState;                                                    // Link from/to this Track?
        bool muted;                                                             // True if the Track is muted.
        bool minimized;                                                         // True if the Track is minimized.
        int transposition;
        const Track* track;                                                     // Nullptr if not new.
        OptionalInt playedLocationLine;
        Selection selection;
    };

    /**
     * Constructor.
     * @param pLine the "middle" line to show.
     * @param pTrackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     */
    explicit TrackView(const int pLine, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata) noexcept :
            AbstractTrackView(pLine, std::move(pTrackViewMetadata))
    {
    }

    /**
     * Sets the displayed data. If some of it is different, it is refreshed.
     * @param transmittedDisplayedData the data to display. The Track may not be present if the same.
     */
    virtual void setDisplayedDataAndRefresh(const TransmittedDisplayedData& transmittedDisplayedData) noexcept = 0;

    /**
     * Updates the Meter.
     * @param periodAndNoiseMeterInput the periods and volume to show.
     */
    virtual void updateMeterAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept = 0;

    /**
     * Sets the effect context text. Contrary to the other data, it is stand-alone.
     * @param text the text to display, possibly empty.
     */
    virtual void setEffectContextText(const juce::String& text) noexcept = 0;

    /** @return the channel index related to this TrackView. */
    virtual int getChannelIndex() const noexcept = 0;
};

}   // namespace arkostracker
