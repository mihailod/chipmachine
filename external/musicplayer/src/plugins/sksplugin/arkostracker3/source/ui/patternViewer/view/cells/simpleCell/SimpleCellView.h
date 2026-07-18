#pragma once

#include "../AbstractCellView.h"
#include "SpecialCellCursorRank.h"

namespace arkostracker 
{

/**
 * A cell showing a number, which can be displayed in decimal or hexadecimal. May be on 2 or 3 digits.
 * It can be used by Line or Special Tracks.
 */
class SimpleCellView final : public AbstractCellView
{
public:
    enum class DisplayIfZero : juce::uint8
    {
        showZeros,
        showDashes,
        dontShow,
    };

    /** The listener to the events of this View. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user clicked on a specific rank of the SpecialCell.
         * @param viewRank the rank (>=0). Handy for the callback. This is NOT the cell index! This is the rank from the top of the displayed Cells.
         * @param cursorRank the rank.
         * @param leftButton true if left Button, false if right.
         * @param shift true if shift is pressed.
         */
        virtual void onUserClickedOnRankOfSpecialTrack(int viewRank, SpecialCellCursorRank cursorRank, bool leftButton, bool shift) = 0;

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
    };

    /** The data to display. */
    class DisplayedData
    {
    public:
        /** Constructor for "nothing to show". */
        DisplayedData() noexcept :
                showSomething(false),
                cursorRank(SpecialCellCursorRank::notPresent),
                cursorOnLine(false),
                primaryHighlight(),
                playedLine(false),
                value(0),
                hexadecimal(false),
                textColor(),
                selected(false)
        {
        }

        /** Constructor with "something" to show (no out of bounds). */
        DisplayedData(const SpecialCellCursorRank pCursorRank, const bool pIsCursorOnLine, const OptionalBool pPrimaryHighlight, const bool pIsPlayedLine,
                      const int pValue, const bool pHexadecimal, const juce::Colour pTextColor, const bool pIsSelected) noexcept :
            showSomething(true),
            cursorRank(pCursorRank),
            cursorOnLine(pIsCursorOnLine),
            primaryHighlight(pPrimaryHighlight),
            playedLine(pIsPlayedLine),
            value(pValue),
            hexadecimal(pHexadecimal),
            textColor(pTextColor),
            selected(pIsSelected)
        {
        }

        bool isShowSomething() const noexcept
        {
            return showSomething;
        }

        SpecialCellCursorRank getCursorRank() const noexcept
        {
            return cursorRank;
        }

        bool isCursorOnLine() const noexcept
        {
            return cursorOnLine;
        }

        const OptionalBool& getPrimaryHighlight() const noexcept
        {
            return primaryHighlight;
        }

        int getValue() const noexcept
        {
            return value;
        }

        bool isHexadecimal() const noexcept
        {
            return hexadecimal;
        }

        const juce::Colour& getTextColor() const noexcept
        {
            return textColor;
        }

        bool isPlayedLine() const noexcept
        {
            return playedLine;
        }

        bool isSelected() const noexcept
        {
            return selected;
        }

        bool operator==(const DisplayedData& rhs) const
        {
            return showSomething == rhs.showSomething &&
                   cursorRank == rhs.cursorRank &&
                   cursorOnLine == rhs.cursorOnLine &&
                   primaryHighlight == rhs.primaryHighlight &&
                   playedLine == rhs.playedLine &&
                   value == rhs.value &&
                   hexadecimal == rhs.hexadecimal &&
                   textColor == rhs.textColor &&
                   selected == rhs.selected;
        }

        bool operator!=(const DisplayedData& rhs) const
        {
            return !(rhs == *this);
        }

    private:
        bool showSomething;
        SpecialCellCursorRank cursorRank;
        bool cursorOnLine;
        OptionalBool primaryHighlight;          // If present, true if primary, false if secondary.
        bool playedLine;                        // True if the line is being played.
        int value;
        bool hexadecimal;
        juce::Colour textColor;
        bool selected;
    };

    /**
     * Constructor.
     * @param listener the listener.
     * @param cellRank the cell rank, useful for the callbacks. This is NOT the cell index!
     * @param font the Font to use. No copy will be made!
     * @param doubleFont the double-height Font to use. No copy will be made!
     * @param displayedData the data to display.
     * @param cellColors the generic colors to be used.
     * @param digitCountIfHexadecimal how many digits there are if hexadecimal digits are displayed.
     * @param digitCountIfDecimal how many digits there are if decimal digits are displayed.
     * @param displayIfZero how to display the zero value.
     */
    SimpleCellView(Listener& listener, int cellRank, const juce::Font& font, const juce::Font& doubleFont, const DisplayedData& displayedData,
                   const CellColors& cellColors, int digitCountIfHexadecimal, int digitCountIfDecimal, DisplayIfZero displayIfZero) noexcept;

    /**
     * @return the width the Cell.
     * @param font the Font to use.
     */
    static int calculateWidth(const juce::Font& font) noexcept;

    /**
     * Sets new data to display. Will refresh the view, if needed.
     * @param newDisplayedData the new data to display.
     */
    void updateDisplayedData(const DisplayedData& newDisplayedData) noexcept;

    // AbstractCell method implementations.
    // =======================================
    void setUseDoubleFontAndRepaint(bool useDoubleFont) noexcept override;
    int getCellRankFromX(int x) const noexcept override;

    // Component method implementations.
    // =====================================
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    static const juce::String twoDashes;

    Listener& listener;

    DisplayedData displayedData;                    // What is displayed.
    const int digitCountIfDecimal;                  // How many digits are displayed if decimal.
    const DisplayIfZero displayIfZero;              // How to display the zeros.
};

}   // namespace arkostracker
