#pragma once

#include "../../editorWithBars/view/component/BarAndCaptionData.h"
#include "../../editorWithBars/view/component/BarAreaSize.h"

namespace arkostracker 
{

/** Utility class to create the values for the Arpeggio Bars, from the model. */
class ArpeggioBarCreator                // TODO TU this!
{
public:
    /**
     * Creates a BarData for the Primary Arpeggio note in octave.
     * @param storedValue the value in this item of the Expression.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createArpeggioNoteInOctaveBarData(int storedValue, bool withinLoop, bool outOfBounds, bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Arpeggio Octave.
     * @param barAreaSize the size of the area.
     * @param storedValue the value in this item of the Expression.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createArpeggioOctaveBarData(BarAreaSize barAreaSize, int storedValue, bool withinLoop, bool outOfBounds, bool hovered,
        bool selected, bool withCursor) noexcept;

private:
    static const juce::Image emptyImage;
};

}   // namespace arkostracker
