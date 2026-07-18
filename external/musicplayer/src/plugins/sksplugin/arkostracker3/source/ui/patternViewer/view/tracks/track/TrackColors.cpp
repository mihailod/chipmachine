#include "TrackColors.h"

#include <utility>

namespace arkostracker 
{

TrackColors::TrackColors(CellColors pCellColors, const juce::Colour& pErrorColor, const juce::Colour& pNoNoteColor) :
        cellColors(std::move(pCellColors)),
        errorColor(pErrorColor),
        noNoteColor(pNoNoteColor)
{
}

}   // namespace arkostracker
