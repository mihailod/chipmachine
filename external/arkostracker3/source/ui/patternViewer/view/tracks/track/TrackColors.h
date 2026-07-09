#pragma once

#include "../../cells/CellColors.h"

namespace arkostracker 
{

/** The colors to be used by all Tracks. They are coming from the look'n'feel. */
class TrackColors
{
public:
    /**
     * Constructor.
     * @param cellColors the colors to be used in each Cells.
     * @param errorColor color if an error is present (unknown effect, illegal instrument, etc.).
     * @param noNoteColor color to use if there is no note.
     */
    TrackColors(CellColors cellColors, const juce::Colour& errorColor, const juce::Colour& noNoteColor);

    const CellColors cellColors;
    const juce::Colour errorColor;
    const juce::Colour noNoteColor;
};

}   // namespace arkostracker

