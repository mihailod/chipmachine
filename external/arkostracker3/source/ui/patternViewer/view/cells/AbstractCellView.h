#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "CellColors.h"
#include "../../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** An abstract Cell, used by "music" Track, but also Special and Line Tracks. */
class AbstractCellView : public juce::Component
{
public:
    static const int borderWidth;                             // The border at the left and right of the texts.

    /**
     * Constructor.
     * @param viewRank the rank (>=0). Handy for the callback. This is NOT the cell index! This is the rank from the top of the displayed Cells.
     * @param font the Font to use. No copy will be made!
     * @param doubleFont the double-height Font to use. No copy will be made!
     * @param cellColors the colors to be used.
     */
    AbstractCellView(int viewRank, const juce::Font& font, const juce::Font& doubleFont, const CellColors& cellColors) noexcept;

    /** @param useDoubleFont true to use the double-height font. This redraws the Cell, if the flag is different. */
    virtual void setUseDoubleFontAndRepaint(bool useDoubleFont) noexcept = 0;

    /**
     * Updates the colors.
     * @param cellColors the new colors.
     * @param repaint true to force a repaint.
     */
    void updateColors(CellColors cellColors, bool repaint) noexcept;

    /**
     * @return how large one char is, for the given font. Or course, it SHOULD be monospace!
     * @param font the Font to use.
     */
    static int calculateCharWidth(const juce::Font& font) noexcept;

    /** @return the view rank (>=0). */
    int getViewRank() const noexcept;
    /**
     * @return the Cell Rank, according to the given X. This is useful when dragging for example.
     * However, the client will have to cast it according to the type of the cell (CellCursorRank, SpecialCellCursorRank, etc.).
     * @param x the X.
     */
    virtual int getCellRankFromX(int x) const noexcept = 0;

protected:
    static const float textColorInterpolation;                // Interpolation rate for the text, when mixed with the highlight color for example.
    static const float cursorColorInterpolation;        // Interpolation rate for the text under the cursor.
    static const float selectionAlpha;

    /**
     * Displays a letter.
     * @param g the Graphics object.
     * @param letterIndex the digit index where to display the effect.
     * @param baselineY the baseline Y where to display the effect.
     * @param height the height to fill.
     * @param letter the letter to display. Only one digit!
     * @param letterColour the color of the letter.
     * @param isCursorOnLetter true if there is the cursor on the letter.
     * @param isCursorOnLine true if there is the cursor on the line.
     * @param isPrimaryHighlight true if primary highlight, false if second, absent if no highlight.
     */
    void displayChar(juce::Graphics& g, int letterIndex, int baselineY, int height, const juce::String& letter, juce::Colour letterColour,
                     bool isCursorOnLetter, bool isCursorOnLine, OptionalBool isPrimaryHighlight) const noexcept;

    /**
     * @return the letter index from X. May be negative if X is (rather abnormal)!
     * @param x the X.
     */
    int getLetterIndexFromX(int x) const noexcept;

    /**
     * Determines a background color.
     * @param showSomething false to show only black (out of bounds). The rest of the parameters will have no effect.
     * @param primaryHighlight if present, true if primary highlight, false if second.
     * @param isCursorPresentOnLine true if the cursor is present on the line.
     * @param isPlayedLine true if the line is being played.
     * @return the color.
     */
    juce::Colour determineBackgroundColor(bool showSomething, OptionalBool primaryHighlight, bool isCursorPresentOnLine, bool isPlayedLine) const noexcept;

    /** @return the baseline for the text. */
    int getBaseLine() const noexcept;

    /** @return the width for a 1-char width, according to the current font. */
    int getWidth1Char() const noexcept;

    bool useDoubleFont;
    const int viewRank;
    std::unique_ptr<CellColors> cellColors;                                 // Group colors to be used.

    const juce::Font& font;                                                 // The font to use when displaying the tracks.
    const juce::Font& doubleFont;                                           // The double-height font to use when displaying the tracks.

private:
    /**
     * Determines the text color, according to the cursor/cursor line/highlights.
     * @param letterColor the initial letter color.
     * @param isCursorOnLetter true if the cursor is on the letter.
     * @param isCursorOnLine true if the cursor is on the line.
     * @param isPrimaryHighlight if present, true if primary highlight, false if second.
     * @return the new color.
     */
    juce::Colour determineBaseTextColor(juce::Colour letterColor, bool isCursorOnLetter, bool isCursorOnLine, OptionalBool isPrimaryHighlight) const noexcept;

    mutable float fontHeightUsedCalculatingWidth1Char;                      // The font size used when the 1-char width is calculated.
    mutable int cachedWidth1Char;                                           // Cached, but warning, must be recalculated if the font changed.
};

}   // namespace arkostracker

