#pragma once

#include "../AbstractTrackView.h"

#include "../../../controller/Selection.h"

namespace arkostracker 
{

/** A "track" that shows only the line numbers. It holds its header. */
class LineTrackView : public AbstractTrackView
{
public:
    /** The listener to the event of this class. */
    class Listener
    {
    public:
        /** Destructor */
        virtual ~Listener() = default;

        /**
         * The user scrolled with the mouse wheel.
         * @param scrollToBottom true to go to the bottom, false to go up.
         */
        virtual void onUserWantsToScrollVerticallyWithMouseWheelOnLineTrack(bool scrollToBottom) = 0;

        /** The user clicked on the height. */
        virtual void onUserClickedOnTrackHeight() = 0;

        /**
         * The user wants to modify the height.
         * @param offset how much to add. May be negative.
         */
        virtual void onUserWantsToModifyTrackHeightWithOffset(int offset) = 0;

        /**
         * Called when the user clicked on a specific rank of the Cell.
         * @param rankFromCenterAsOrigin the rank, where 0 is the center. This is NOT the cell index! May be negative.
         * @param leftButton true if left Button, false if right.
         */
        virtual void onUserClickedOnLineTrack(int rankFromCenterAsOrigin, bool leftButton) = 0;

        /**
         * Called when the user click is now up. Only called after a down has been clicked.
         * @param leftButton true if left Button, false if right.
         */
        virtual void onUserClickIsUpFromLineTrack(bool leftButton) = 0;

        /**
         * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
         * It is up to the client to find it.
         * @param event the mouse event.
         */
        virtual void onUserDraggedCursorFromLineTrack(const juce::MouseEvent& event) = 0;
    };

    /** What is displayed. */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param pLine the "middle" line.
         * @param pHeight the height to display.
         * @param pHexadecimal true if display hexadecimal numbers.
         * @param pPlayedLocationLine present if a line is being played.
         * @param pSelection the possible selection
         */
        explicit DisplayedData(const int pLine, const int pHeight, const bool pHexadecimal, const OptionalInt& pPlayedLocationLine, const Selection& pSelection) noexcept:
                line(pLine),
                height(pHeight),
                hexadecimal(pHexadecimal),
                playedLocationLine(pPlayedLocationLine),
                selection(pSelection)
        {
            // Only Line TrackType is allowed here!
            jassert(selection.isEmpty() || selection.getStartLocation().getTrackType() == TrackType::line);
            jassert(selection.isEmpty() || selection.getEndLocation().getTrackType() == TrackType::line);
        }

        int getLine() const noexcept
        {
            return line;
        }

        int getHeight() const noexcept
        {
            return height;
        }

        bool isHexadecimal() const noexcept
        {
            return hexadecimal;
        }

        const OptionalInt& getPlayedLocationLine() const noexcept
        {
            return playedLocationLine;
        }

        const Selection& getSelection() const
        {
            return selection;
        }

        bool operator==(const DisplayedData& rhs) const
        {
            return line == rhs.line &&
                   height == rhs.height &&
                   hexadecimal == rhs.hexadecimal &&
                   playedLocationLine == rhs.playedLocationLine &&
                   selection == rhs.selection;
        }

        bool operator!=(const DisplayedData& rhs) const
        {
            return !(rhs == *this);
        }

    private:
        int line;
        int height;
        bool hexadecimal;
        OptionalInt playedLocationLine;
        Selection selection;
    };

    /**
     * Constructor.
     * @param pLine the "middle" line to show.
     * @param pTrackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     */
    explicit LineTrackView(const int pLine, std::shared_ptr<TrackViewMetadata> pTrackViewMetadata) noexcept :
            AbstractTrackView(pLine, std::move(pTrackViewMetadata))
    {
    }

    /**
     * Sets the data to display. This refreshes the UI if needed.
     * @param displayedData the data to display.
     */
    virtual void setDisplayedData(const DisplayedData& displayedData) noexcept = 0;
};

}   // namespace arkostracker
