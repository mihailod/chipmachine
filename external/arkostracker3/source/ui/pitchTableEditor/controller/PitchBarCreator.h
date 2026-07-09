#pragma once

#include "../../editorWithBars/view/component/BarAndCaptionData.h"
#include "../../editorWithBars/view/component/BarAreaSize.h"

namespace arkostracker 
{

/** Utility class to create the values for the Pitch Bars, from the model. */
class PitchBarCreator
{
public:
    /**
     * Creates a BarData for the Primary Arpeggio note in octave.
     * @param areaSize the current size of the area.
     * @param storedValue the value in this item of the Expression.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPitchBarData(BarAreaSize areaSize, int storedValue, bool withinLoop, bool outOfBounds, bool hovered, bool selected, bool withCursor) noexcept;

private:
    static const juce::Image emptyImage;
};

}   // namespace arkostracker
