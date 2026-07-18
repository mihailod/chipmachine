#pragma once

#include <unordered_map>

#include "../../../../../song/cells/Note.h"
#include "../../../../../utils/OptionalValue.h"
#include "../AbstractCellView.h"
#include "../CellColors.h"
#include "CellCursorRank.h"

namespace arkostracker 
{

/**
 * The View for a "Cell" of a normal Track.
 *
 * Note that a selection cannot select ranks that are minimized. It is up to the client to correct the selection
 * and select the right ranks.
 */
class CellView final : public AbstractCellView
{
public:

    /** The listener to the events of this View. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user clicked on a specific rank of the Cell.
         * @param viewRank the rank (>=0). Handy for the callback. This is NOT the cell index! This is the rank from the top of the displayed Cells.
         * @param cursorRank the rank.
         * @param leftButton true if left Button, false if right.
         * @param shift true if shift is pressed.
         */
        virtual void onUserClickedOnRank(int viewRank, CellCursorRank cursorRank, bool leftButton, bool shift) = 0;

        /**
         * Called when the user click is now up. Only called after a down has been clicked.
         * @param leftButton true if left Button, false if right.
         */
        virtual void onUserClickIsUp(bool leftButton) = 0;

        /**
         * Called when the user dragged the cursor. Since it can drag to any other Cell this view doesn't know, it cannot indicate the view rank and cursor rank.
         * It is up to the client to find it.
         * @param event the mouse event.
         */
        virtual void onUserDraggedCursor(const juce::MouseEvent& event) = 0;
    };


    // ===================================================================

    /** Holds an effect for display. Immutable. */
    class DisplayedEffect
    {
    public:
        DisplayedEffect(juce::juce_wchar effect, int value, int valueDigitCount, juce::Colour color);

        juce::juce_wchar getEffect() const noexcept;
        int getValue() const noexcept;
        int getValueDigitCount() const noexcept;
        const juce::Colour& getColor() const noexcept;

        bool operator==(const DisplayedEffect& rhs) const;        // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const DisplayedEffect& rhs) const;        // NOLINT(fuchsia-overloaded-operator)

    private:
        juce::juce_wchar effect;            // The shown effect.
        int value;                          // The value (1, 2 or 3 digits).
        int valueDigitCount;                // 1, 2 or 3.

        juce::Colour color;
    };

    /** Holds the data and flags to display. Immutable! */
    class DisplayedData
    {
    public:
        /** Constructor for nothing to be displayed (useful for out of bounds). */
        DisplayedData() noexcept;

        /** Constructor for "something to show". */
        DisplayedData(const OptionalValue<Note>& note, const OptionalInt& instrument,
                      juce::Colour noteColor, juce::Colour instrumentColor,
                      std::unordered_map<int, DisplayedEffect> indexToEffects, bool isCursorOnLine, CellCursorRank cursorPosition,
                      CellCursorRank startSelection, CellCursorRank endSelection,
                      bool minimized,
                      OptionalValue<bool> primaryHighlight, bool isPlayedLine) noexcept;

        bool isShowSomething() const noexcept;
        const OptionalValue<Note>& getNote() const noexcept;
        const OptionalInt& getInstrument() const noexcept;
        bool isCursorOnLine() const noexcept;
        CellCursorRank getCursorPosition() const noexcept;
        const std::unordered_map<int, CellView::DisplayedEffect>& getEffects() const noexcept;
        CellCursorRank getStartSelection() const noexcept;
        CellCursorRank getEndSelection() const noexcept;
        const juce::Colour& getNoteColor() const noexcept;
        const juce::Colour& getInstrumentColor() const noexcept;
        const OptionalValue<bool>& getPrimaryHighlight() const noexcept;
        bool isPlayedLine() const noexcept;

        bool operator==(const DisplayedData& rhs) const;        // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const DisplayedData& rhs) const;        // NOLINT(fuchsia-overloaded-operator)

        /** @return true if the effects are collapsed. */
        bool areEffectsCollapsed() const noexcept;

    private:
        bool showSomething;                                     // If false, nothing is shown (for out-of bound cell).
        OptionalValue<Note> note;                               // A possible note.
        OptionalInt instrument;                                 // An instrument, if present.

        std::unordered_map<int, DisplayedEffect> indexToEffects;    // The possible effects. Only non-null must be present.

        bool cursorOnLine;                                      // True if the line must be highlighted because the cursor is present, MAYBE on ANOTHER Track!!
        CellCursorRank cursorPosition;                          // Where the cursor is, if any ("not present" possible, even if cursorOnLine is true).
        CellCursorRank startSelection;                          // Where the possible selection starts ("not present" possible).
        CellCursorRank endSelection;                            // Where the possible selection ends. Only relevant is the startSelection is also present ("not present" possible).
        juce::Colour noteColor;
        juce::Colour instrumentColor;
        bool minimized;                                         // Simplified management: all if nothing is minimized.
        OptionalValue<bool> primaryHighlight;                   // If not present, no highlight. True if primary highlight, false if secondary highlight.
        bool playedLine;                                        // True if the line is being played.
    };


    // ===================================================================

    /**
     * Constructor.
     * @param listener the listener.
     * @param viewRank the rank (>=0). Handy for the callback. This is NOT the cell index! This is the rank from the top of the displayed Cells.
     * @param font the Font to use. No copy will be made!
     * @param doubleFont the double-height Font to use. No copy will be made!
     * @param displayedData the data to display.
     * @param cellColors the colors to be used.
     */
    CellView(Listener& listener, int viewRank, const juce::Font& font, const juce::Font& doubleFont, DisplayedData displayedData, const CellColors& cellColors) noexcept;

    /**
     * @return the width the Cell should have, according to how many effects are minimized.
     * @param minimizedEffectCount how many effects are minimized.
     * @param font the Font to use.
     */
    static int calculateWidth(int minimizedEffectCount, const juce::Font& font) noexcept;

    /**
     * Updates the UI with the given data, if necessary.
     * @param displayedData the data to display.
     */
    void updateDisplayedData(const DisplayedData& displayedData) noexcept;

    // AbstractCell method implementations.
    // =======================================
    void setUseDoubleFontAndRepaint(bool useDoubleFont) noexcept override;
    int getCellRankFromX(int x) const noexcept override;

    // Component method implementations.
    // =====================================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    static const juce::String oneDash;                          // Shows "-".
    static const juce::String twoDashes;                        // Shows "--".
    static const juce::String threeDashes;                      // Shows "---".
    static const juce::String fourDashes;                       // Shows "----".
    static const juce::String rst;
    static const juce::String twoSpaces;
    static const juce::String sameInstrument;                   // Used for legato.

    /**
     * Displays an effect.
     * @param g the Graphics object.
     * @param letterIndex the digit index where to display the effect.
     * @param baselineY the baseline Y where to display the effect.
     * @param height the height to fill.
     * @param effectIndex the index of the effect.
     * @return the letter index after the effect.
     */
    int displayEffect(juce::Graphics& g, int letterIndex, int baselineY, int height, int effectIndex) const noexcept;

    /**
     * Fills the maps between a CursorRank and its index in the letters.
     * @param isMinimized true if the effects are minimized.
     * @param letterIndexToCursorRank the map to fill.
     * @param cursorRankToLetterIndex the map to fill.
     */
    static void buildCursorRankAndLetterIndexMaps(bool isMinimized, std::unordered_map<int, CellCursorRank>& letterIndexToCursorRank,
                                                  std::unordered_map<CellCursorRank, int>& cursorRankToLetterIndex) noexcept;

     /**
      * @return true if there is a cursor under the given letter index.
      * @param letterIndex the index of the letter.
      * @param cursorRank the rank of the cursor. If "no present", false if returned.
      */
    bool isCursorOnLetter(int letterIndex, CellCursorRank cursorRank) const noexcept;

    /**
     * @return the clicked Rank. It will always a valid rank.
     * @param x the X.
     */
    CellCursorRank getRankFromX(int x) const noexcept;

    /**
     * @return the letter index to cursor rank map to use.
     * @param isMinimized true to use the map when minimized, false when maximized.
     */
    const std::unordered_map<int, CellCursorRank>& getLetterIndexToCursorRank(bool isMinimized) const noexcept;

    /**
     * @return the cursor rank to letter index map to use.
     * @param isMinimized true to use the map when minimized, false when maximized.
     */
    const std::unordered_map<CellCursorRank, int>& getCursorRankToLetterIndex(bool isMinimized) const noexcept;

    /**
     * Maps the given rank to a rank that is visible when the channel is minimized. May return the exact same rank.
     * Effect digits are mapped into the effect number.
     * @param rank the rank.
     * @return the mapped rank.
     */
    static CellCursorRank mapRankToMinimizedRank(CellCursorRank rank) noexcept;

    Listener& listener;
    DisplayedData displayedData;

    std::unordered_map<int, CellCursorRank> letterIndexToCursorRankMinimized;        // Links a letter index to the possible CursorRank, when minimized.
    std::unordered_map<int, CellCursorRank> letterIndexToCursorRankMaximized;        // Links a letter index to the possible CursorRank, when maximized.
    std::unordered_map<CellCursorRank, int> cursorRankToLetterIndexMinimized;        // Links a CursorRank to the possible letter index, when minimized.
    std::unordered_map<CellCursorRank, int> cursorRankToLetterIndexMaximized;        // Links a CursorRank to the possible letter index, when maximized.
};

}   // namespace arkostracker
