#pragma once

#include "CaptionedBar.h"
#include "../AreaType.h"

namespace arkostracker 
{

class BarData;
class BarAreaController;

/**
 * Holds bars, maybe of different kind. This is typically put into a Viewport.
 * Interactions with the bars are also managed here.
 *
 * May display something below each bar.
 */
class BarArea final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param areaType the type of the area.
     * @param controller the controller to the Area.
     * @param barWidth how large are every bar.
     */
    BarArea(AreaType areaType, BarAreaController& controller, int barWidth) noexcept;

    /** Destructor. */
    ~BarArea() override;

    /** @return the type of the area. */
    AreaType getAreaType() const noexcept;

    /**
     * @return the value of the given bar index.
     * @param barIndex the bar index. If invalid, 0 is returned.
     */
    int getValue(int barIndex) const noexcept;

    /**
     * Sets the bar count. Bars may be removed, or created. Created bars will have default empty values.
     * @param barCount how many bars.
     */
    void setDisplayedBarCount(int barCount) noexcept;

    /**
     * Sets the data of a Bar. Nothing happens if the bar has the same data.
     * @param index the index of the bar.
     * @param barData the data of the Bar.
     * @param captionData the data of the Caption.
     */
    void setBarData(int index, const BarData& barData, const BarCaptionDisplayedData& captionData) const noexcept;

    /** Sets the bar width, refreshes the UI if needed. */
    void setBarWidth(int barWidth) noexcept;

    /**
     * @return the X of the bar.
     * @param index the index of the bar.
     */
    int getBarX(int index) const noexcept;

    /** @return the width of one bar. */
    int getBarWidth() const noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static const int separatorWidth;                                    // Width between bars (actually, OVER the original width).
    static const int minimumHeightBeforeConsideredHidden;

    /**
     * @return the CaptionedBar that is selected, or nullptr if there is none.
     * @param event the mouse event.
     */
    CaptionedBar* getCaptionedBarFromMouseEvent(const juce::MouseEvent& event) noexcept;

    /**
     * @return the Bar that is selected, or nullptr if there is none.
     * @param event the mouse event.
     * @param isDrag true if a drag being performed. False if a click down.
     */
    Bar* getBarFromMouseEvent(const juce::MouseEvent& event, bool isDrag) noexcept;

    /**
     * A click has been done. Manages the event.
     * @param event the mouse event.
     * @param isDrag true if a drag being performed. False if a click down.
     */
    void manageClick(const juce::MouseEvent& event, bool isDrag) noexcept;

    /**
     * A wheel event has been performed. Manages the event.
     * @param event the mouse event.
     * @param wheel the wheel details.
     * @return true if the event has been consumed.
     */
    bool manageWheel(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) noexcept;

    /** Locates the resize the bars. This should be called whenever a new bar is added, or if a new bar width is set. */
    void locateAndResizeBars() noexcept;

    /** @return true if the component is considered hidden (because too small). */
    bool isHidden() const noexcept;
    /** Sets the location/size/visibility of the placeholder. Should be called on resize, but also when the bar count changes. */
    void locatePlaceholder() noexcept;

    const AreaType areaType;
    BarAreaController& controller;
    int barWidthWithSpacing;

    std::vector<std::shared_ptr<CaptionedBar>> captionedBars;
    BlankComponent hiddenPlaceholder;                       // Shown when the height is too small, considered "hidden".

    juce::int64 latestWheelEventTimestamp;                  // When the latest wheel event happened, to prevent multiple events.
};

}   // namespace arkostracker
