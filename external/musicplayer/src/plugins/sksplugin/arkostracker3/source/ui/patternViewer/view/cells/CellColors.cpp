#include "CellColors.h"

namespace arkostracker 
{

CellColors::CellColors(const juce::Colour& pBackgroundColor, const juce::Colour& pNoEffectColor, const juce::Colour& pCursorBackgroundColor,
                       const juce::Colour& pCursorTextHighlightColor, const juce::Colour& pSelectionColor,
                       const juce::Colour& pLinePrimaryHighlightBackgroundColor, const juce::Colour& pLineSecondaryHighlightBackgroundColor,
                       const juce::Colour& pLinePrimaryInterpolatedHighlightTextColor, const juce::Colour& pLineSecondaryInterpolatedHighlightTextColor,
                       const juce::Colour& pCursorLineInterpolatedBackgroundColor, const juce::Colour& pCursorLineInterpolatedHighlightTextColor,
                       const juce::Colour& pPlayedLineBackgroundColor) :
        backgroundColor(pBackgroundColor),
        noEffectColor(pNoEffectColor),
        cursorBackgroundColor(pCursorBackgroundColor),
        cursorTextHighlightColor(pCursorTextHighlightColor),
        selectionColor(pSelectionColor),
        linePrimaryHighlightBackgroundColor(pLinePrimaryHighlightBackgroundColor),
        lineSecondaryHighlightBackgroundColor(pLineSecondaryHighlightBackgroundColor),
        linePrimaryInterpolatedHighlightTextColor(pLinePrimaryInterpolatedHighlightTextColor),
        lineSecondaryInterpolatedHighlightTextColor(pLineSecondaryInterpolatedHighlightTextColor),
        cursorLineInterpolatedBackgroundColor(pCursorLineInterpolatedBackgroundColor),
        cursorLineInterpolatedHighlightTextColor(pCursorLineInterpolatedHighlightTextColor),
        playedLineBackgroundColor(pPlayedLineBackgroundColor)
{
}

}   // namespace arkostracker

