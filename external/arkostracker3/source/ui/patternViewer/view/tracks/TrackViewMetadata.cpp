#include "TrackViewMetadata.h"

namespace arkostracker
{

TrackViewMetadata::TrackViewMetadata(const int pHighlightStep, const int pSecondaryHighlight,
                                     const bool pZoomMiddleLine,
                                     std::unordered_map<Effect, juce::juce_wchar> pEffectToChar,
                                     std::unordered_map<Effect, juce::Colour> pEffectToColor,
                                     std::unordered_map<int, juce::uint32> pInstrumentToColorArgb,
                                     const int pArpeggioExpressionCount, const int pPitchExpressionCount
) :
        highlightStep(pHighlightStep),
        secondaryHighlight(pSecondaryHighlight),
        zoomMiddleLine(pZoomMiddleLine),
        effectToChar(std::move(pEffectToChar)),
        effectToColor(std::move(pEffectToColor)),
        instrumentToColorArgb(std::move(pInstrumentToColorArgb)),
        arpeggioExpressionCount(pArpeggioExpressionCount),
        pitchExpressionCount(pPitchExpressionCount)
{
    jassert(highlightStep > 0);
    jassert(pSecondaryHighlight > 0);
}

}   // namespace arkostracker
