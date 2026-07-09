#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Shows a possible timeline with a possible cursor. */
class PlayLineView final : public juce::Component
{
public:

    /** Holds the data and flags to display. Immutable! */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param pPositionCount how many positions to show.
         * @param pPlayStartPosition the position where the play begins.
         * @param pEndPosition the position where the play ends/loops.
         * @param pCurrentPosition where the cursor is.
         * @param pCurrentPatternHeight how many lines are present for the pattern where the cursor is.
         * @param pCurrentLine the line currently being played in the position.
         */
        DisplayedData(int pPositionCount, int pPlayStartPosition, int pEndPosition, int pCurrentPosition, int pCurrentPatternHeight, int pCurrentLine) noexcept :
                positionCount(pPositionCount),
                playStartPosition(pPlayStartPosition),
                endPosition(pEndPosition),
                currentPosition(pCurrentPosition),
                currentPatternHeight(pCurrentPatternHeight),
                currentLine(pCurrentLine)
        {
        }

        int getPositionCount() const noexcept
        {
            return positionCount;
        }

        int getPlayStartPosition() const noexcept
        {
            return playStartPosition;
        }

        int getEndPosition() const noexcept
        {
            return endPosition;
        }

        int getCurrentPosition() const noexcept
        {
            return currentPosition;
        }

        int getCurrentPatternHeight() const noexcept
        {
            return currentPatternHeight;
        }

        int getCurrentLine() const noexcept
        {
            return currentLine;
        }

        bool operator==(const DisplayedData& rhs) const                // NOLINT(fuchsia-overloaded-operator)
        {
            return positionCount == rhs.positionCount &&
                   playStartPosition == rhs.playStartPosition &&
                   endPosition == rhs.endPosition &&
                   currentPosition == rhs.currentPosition &&
                   currentPatternHeight == rhs.currentPatternHeight &&
                   currentLine == rhs.currentLine;
        }

        bool operator!=(const DisplayedData& rhs) const                // NOLINT(fuchsia-overloaded-operator)
        {
            return !(rhs == *this);
        }

    private:
        int positionCount;
        int playStartPosition;
        int endPosition;
        int currentPosition;
        int currentPatternHeight;
        int currentLine;
    };

    /**
     * Constructor.
     * @param itemWidth the width of each item (position).
     */
    explicit PlayLineView(int itemWith) noexcept;

    /**
     * Refreshes the UI, if necessary.
     * @param displayedData the data to display.
     */
    void refreshUi(const DisplayedData& displayedData) noexcept;

    // Component method implementations.
    // ======================================
    void paint(juce::Graphics& g) override;
    void lookAndFeelChanged() override;

private:
    static const int playLineHeight;                    // Also determines the height of the cursor.
    static const float cursorWidth;
    static const float dashLengths[];                   // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays)

    /** @return the color used to draw the line. */
    static juce::Colour findLineColor() noexcept;

    /** @return the full width of the line, according to the current displayed data. */
    int getLineWidth() const noexcept;

    int itemWidth;                                      // How large is one item.
    DisplayedData displayedData;
    juce::Colour lineColor;

};


}   // namespace arkostracker

