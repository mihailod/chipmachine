#pragma once

#include "Bar.h"
#include "BarCaption.h"

namespace arkostracker 
{

/** Represents a Bar, with a Caption below. */
class CaptionedBar final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param barIndex a convenient index to identify the bar.
     */
    explicit CaptionedBar(int barIndex) noexcept;

    /**
     * Sets the value of the bar. Refreshes the UI, if the value has changed.
     * @param barData the data to display.
     * @param captionDisplayData the data to show on the caption.
     */
    void setDisplayedData(const BarData& barData, const BarCaptionDisplayedData& captionDisplayData) noexcept;

    /** @return the index of the Bar, as given by the constructor. */
    int getIndex() const noexcept;

    /** @return the currently displayed value. */
    int getCurrentValue() const noexcept;

    /**
     * @return the value related to the given Y. Always within limit.
     * @param y the Y. Should be from within the display limit.
     */
    int determineValue(int y) const noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    Bar bar;
    BarCaption caption;
};

}   // namespace arkostracker

