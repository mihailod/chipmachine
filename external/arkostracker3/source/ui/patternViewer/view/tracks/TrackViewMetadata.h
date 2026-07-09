#pragma once

#include <unordered_map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../song/cells/Effect.h"

namespace arkostracker 
{

/**
 * Data besides the Track to display, the TrackView will require to display properly. These are NOT colors from the look'n'feel.
 * This data is shared among all Tracks, so specific data are not shown here.
 */
class TrackViewMetadata
{
public:
    TrackViewMetadata(int highlightStep, int pSecondaryHighlight, bool zoomMiddleLine, std::unordered_map<Effect, juce::juce_wchar> effectToChar,
                      std::unordered_map<Effect, juce::Colour> effectToColor, std::unordered_map<int, juce::uint32> instrumentToColorArgb,
                      int arpeggioExpressionCount, int pitchExpressionCount);

    const int highlightStep;                                                      // >0 (>1 in practice). How much steps between each highlight.
    const int secondaryHighlight;                                                 // >0 (>1 in practice). Every X highlight is a secondary highlight (probably more focused).
    const bool zoomMiddleLine;                                                    // True to zoom the middle line.
    const std::unordered_map<Effect, juce::juce_wchar> effectToChar;              // Links an effect to a displayable char.
    const std::unordered_map<Effect, juce::Colour> effectToColor;                 // Links an effect to its color.
    const std::unordered_map<int, juce::uint32> instrumentToColorArgb;                // Links an instrument to its color.
    const int arpeggioExpressionCount;                                            // How many Arpeggio the Song has.
    const int pitchExpressionCount;                                               // How many Pitch the Song has.
};

}   // namespace arkostracker
