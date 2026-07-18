#pragma once

#include "../../../components/BlankComponent.h"
#include "../../../components/ColoredImage.h"
#include "BarCaptionDisplayedData.h"

namespace arkostracker 
{

/**
 * Small caption put below each bar. Can contain:
 * - A possible main image.
 * - A possible text.
 */
class BarCaption final : public juce::Component
{
public:

    /** Constructor. */
    BarCaption() noexcept;

    /**
     * Sets the data to display. Refreshes the UI, if there is some changes.
     * @param displayedData the data to display.
     */
    void setDisplayedData(const BarCaptionDisplayedData& displayedData) noexcept;

    /** @return true if at least a text or an image is present. */
    bool isDataPresent() const noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;
    void lookAndFeelChanged() override;

private:
    static const int maxHeightImage;
    static const int maxHeightMainText;
    static const int fontHeight;
    static const juce::uint8 backgroundColorAlpha;
    static const int minimumCaptionWidthForVisibility;          // If smaller, the caption is hidden.

    /** Sets the items locations according to the state. */
    void locateItems() noexcept;

    /** Sets the image color according to the LookAndFeel. */
    void setImageComponentColor() noexcept;

    BarCaptionDisplayedData displayedData;

    BlankComponent background;
    ColoredImage imageComponent;
    juce::Label mainLabel;
    juce::Font textFont;
};

}   // namespace arkostracker
