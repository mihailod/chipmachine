#pragma once

#include <juce_graphics/juce_graphics.h>

namespace arkostracker 
{

/** Groups the colors used by the CellView. This is shared among all CellViews, so this represent generic colors. */
class CellColors
{
public:
    /**
     * Constructor.
     * @param backgroundColor the background color.
     * @param noEffectColor the color if there is no effect.
     * @param cursorBackgroundColor the color of the cursor, in the background.
     * @param cursorTextHighlightColor the color of the highlight color for the text.
     * @param selectionColor the selection color.
     * @param linePrimaryHighlightBackgroundColor the primary background color for the highlight, if any.
     * @param lineSecondaryHighlightBackgroundColor the secondary background color for the highlight, if any.
     * @param linePrimaryInterpolatedHighlightTextColor the primary interpolated color for the line highlight, if any.
     * @param linePrimaryInterpolatedHighlightTextColor the secondary interpolated color for the line highlight, if any.
     * @param cursorLineInterpolatedBackgroundColor the interpolated background color if the cursor is present.
     * @param cursorLineInterpolatedHighlightTextColor the interpolation color for the text, when a line has the cursor, if any.
     * @param playedLineBackgroundColor the interpolation color for the background, when the line is being played.
     */
    CellColors(const juce::Colour& backgroundColor, const juce::Colour& noEffectColor, const juce::Colour& cursorBackgroundColor,
               const juce::Colour& cursorTextHighlightColor, const juce::Colour& selectionColor,
               const juce::Colour& linePrimaryHighlightBackgroundColor, const juce::Colour& lineSecondaryHighlightBackgroundColor,
               const juce::Colour& linePrimaryInterpolatedHighlightTextColor, const juce::Colour& lineSecondaryInterpolatedHighlightTextColor,
               const juce::Colour& cursorLineInterpolatedBackgroundColor, const juce::Colour& cursorLineInterpolatedHighlightTextColor,
               const juce::Colour& playedLineBackgroundColor);

    const juce::Colour backgroundColor;
    const juce::Colour noEffectColor;
    const juce::Colour cursorBackgroundColor;
    const juce::Colour cursorTextHighlightColor;
    const juce::Colour selectionColor;
    const juce::Colour linePrimaryHighlightBackgroundColor;
    const juce::Colour lineSecondaryHighlightBackgroundColor;
    const juce::Colour linePrimaryInterpolatedHighlightTextColor;
    const juce::Colour lineSecondaryInterpolatedHighlightTextColor;
    const juce::Colour cursorLineInterpolatedBackgroundColor;
    const juce::Colour cursorLineInterpolatedHighlightTextColor;
    const juce::Colour playedLineBackgroundColor;
};

}   // namespace arkostracker

