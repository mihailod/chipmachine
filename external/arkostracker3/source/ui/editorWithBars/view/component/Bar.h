#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "BarData.h"

namespace arkostracker 
{

/**
 * A simple bar, to be displayed in any Bar Editor (IE, Expression editor).
 * As unidirectional flow dictates, this view is refreshed by a single immutable BarData object.
 *
 * IMPORTANT: this View does not manage ANY event at all, because it prevents us from
 * doing things like click + draw on several other bars. So the parent managed all the events.
 */
class Bar final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param barIndex a convenient index to identify the bar.
     */
    explicit Bar(int barIndex) noexcept;

    /**
     * Sets the value of the bar. Refreshes the UI, if the value has changed.
     * @param barData the data to display.
     */
    void setDisplayedData(const BarData& barData) noexcept;

    /** @return the index of the Bar, as given by the constructor. */
    int getIndex() const noexcept;

    /**
     * @return the value related to the given Y. Always within limit.
     * @param y the Y. Should be from within the display limit.
     */
    int determineValue(int y) const noexcept;

    /** @return the value currently displayed. */
    int getCurrentValue() const noexcept;

    // Components method implementations.
    // ======================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void lookAndFeelChanged() override;

private:
    static const float colorContrastRatioHover;                 // Contrast when hovering the bar with the mouse.
    static const float colorContrastRatioBackgroundHover;
    static const float colorContrastRatioSelected;              // Contrast when the bar is selected.
    static const float generatedWidthRatio;                     // Ratio to shrink the width of a bar, if generated.
    static const float outsideLoopAlphaMultiplier;              // Multiplier on the alpha for a bar, if outside the loop.
    static const float ignoredAlphaMultiplier;                  // Multiplier on the alpha for a bar, if ignored.
    static const float generatedAlphaMultiplier;                // Multiplier on the alpha for a bar, if generated.

    /** Simple holder of the value display data. */
    class DisplayedValue
    {
    public:
        DisplayedValue(const int pTop, const int pHeight, const bool pOutOfVisibleMinMax) :
                top(pTop),
                height(pHeight),
                outOfVisibleMinMax(pOutOfVisibleMinMax)
        {
        }

        int getTop() const noexcept
        {
            return top;
        }

        int getHeight() const noexcept
        {
            return height;
        }

        bool isOutOfVisibleMinMax() const noexcept
        {
            return outOfVisibleMinMax;
        }

    private:
        int top;
        int height;
        /** True when the value is beyond the visible min/max (but still in the value's limit). */
        bool outOfVisibleMinMax;
    };

    /**
     * Calculates the display ratio according to the current height and value range. This must be
     * called every time these value changes.
     */
    void calculateDisplayRatio() noexcept;

    /**
     * Updates the internal colors from the current color ids.
     * @return true if the stored colors are now different.
     */
    bool updateColorsFromCurrentData() noexcept;

    /** @return the only instance of the Retrig image. */
    static const juce::Image& getRetrigImage() noexcept;

    /**
     * Displays the cursor, if needed.
     * @param g the Graphics object.
     */
    void displayCursor(juce::Graphics& g) const noexcept;

    const int barIndex;

    BarData displayedData;

    juce::Colour baseBarColor;              // The colors are determined when the data change, or on look'n'feel change.
    juce::Colour baseBarBackgroundColor;

    int displayRange;                   // How many values can be displayed. Does not depend on the min/max values, but on the displayed min/max values!
    double displayRatio;                // Ratio on Y, according to the height.

    juce::Path topTrianglePath;
    juce::Path pathBackground;

public:
    /** @return the Y and height of the bar to display, according to the current value and bounds. */
    DisplayedValue calculateBarTopAndHeight() const noexcept;           // Not great, should be private... Useful for testing.
};

}   // namespace arkostracker
