#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/OptionalValue.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

/**
 * Represents a loop bar, along with indexes if wanted. It is passive.
 * However, the hovering is still managed here.
 */
class LoopBar final : public juce::Component
{
public:
    static const float markerSize;                          // Can be used to define the height.

    /** The listener for clients to be aware of the events. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user wants to set the start of the loop bar.
         * @param id the ID of the bar.
         * @param newStart the new start.
         */
        virtual void onUserWantsToSetLoopBarStart(int id, int newStart) noexcept = 0;

        /**
         * Called when the user wants to set the end of the loop bar.
         * @param id the ID of the bar.
         * @param newEnd the new end.
         */
        virtual void onUserWantsToSetLoopBarEnd(int id, int newEnd) noexcept = 0;

        /**
         * Called when the user wants to toggle the state of the loop bar.
         * @param id the ID of the bar.
         */
        virtual void onUserWantsToToggleLoop(int id) noexcept = 0;
    };

    /** The displayed data. */
    class DisplayData
    {
    public:
        /**
         * Constructor.
         * @param pLoopEnabled true if the loop is enabled.
         * @param pStartIndex the start index.
         * @param pEndIndex the end index.
         */
        DisplayData(const bool pLoopEnabled, const int pStartIndex, const int pEndIndex) noexcept :
                loopEnabled(pLoopEnabled),
                startIndex(pStartIndex),
                endIndex(pEndIndex)
        {
            jassert(startIndex <= endIndex);
        }

        bool isLoopEnabled() const noexcept
        {
            return loopEnabled;
        }

        int getStartIndex() const noexcept
        {
            return startIndex;
        }

        int getEndIndex() const noexcept
        {
            return endIndex;
        }

        bool operator==(const DisplayData& rhs) const noexcept
        {
            return loopEnabled == rhs.loopEnabled &&
                   startIndex == rhs.startIndex &&
                   endIndex == rhs.endIndex;
        }

        bool operator!=(const DisplayData& rhs) const noexcept
        {
            return !(rhs == *this);
        }

    private:
        bool loopEnabled;
        int startIndex;
        int endIndex;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param id the ID of the bar.
     * @param itemWidth the width of the "item".
     * @param endMargin If >0, the end marker will be shifted to the left of this amount of pixels.
     * @param showIndexes true to show the indexes, as text.
     * @param barColorId the color ID of the bar.
     */
    LoopBar(Listener& listener, int id, int itemWidth, int endMargin, bool showIndexes, LookAndFeelConstants::Colors barColorId) noexcept;

    /**
     * Sets the display data. Refreshes the UI. If no change, nothing happens.
     * @param newDisplayData the data to display.
     */
    void setDisplayData(const DisplayData& newDisplayData) noexcept;

    /**
     * Sets the width of each item, refreshes the UI.
     * @param itemWidth the width, in pixels.
     */
    void setItemWidth(int itemWidth) noexcept;

    /**
     * Sets the highlighted indexes. Updates the UI.
     * @param indexes the indexes. May be empty.
     */
    void setHighlightedIndexes(const std::vector<int>& indexes) noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;
    void paint(juce::Graphics &g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void lookAndFeelChanged() override;

private:
    static const float markerBarHeight;
    static const int labelsY;
    static const int labelsHeight;
    static const float labelsAlphaOutOfBounds;
    static const float disabledAlpha;                               // Alpha used when the loop is disabled.
    static const float hoveredAlpha;                                // Alpha used when a marker is hovered.
    static const float hoveredSaturationMultiplier;                 // Saturation multiplier used when a marker is hovered.
    static const float highlightItemAlpha;
    static const int highlightHeight;
    static const int highlightYTop;
    /**
     * @return the X (start and end) position of a marker.
     * @param index the index of the marker.
     * @param isStartMarker true if a start marker, false if an end marker.
     */
    std::pair<float, float> calculateMarkerPosition(int index, bool isStartMarker) const noexcept;

    /**
     * Draws a marker.
     * @param g the Graphics.
     * @param path the path to clear and use. Only to avoid to create an object.
     * @param xStartAndXEnd the X of the left and right.
     * @param yBottom the Y at the bottom.
     * @param isStartMarker true if a start marker, false if an end marker.
     */
    static void drawMarker(const juce::Graphics& g, juce::Path& path, const std::pair<float, float>& xStartAndXEnd, float yBottom, bool isStartMarker) noexcept;

    /** Updates the alpha of the labels according to the current states. */
    void updateLabelsAlpha() const noexcept;

    /**
     * Sets the alpha of a label, according its index.
     * @param label the label.
     * @param index the index of the label.
     * @param endIndex the current end index.
     */
    static void setLabelAlpha(juce::Label& label, int index, int endIndex) noexcept;

    /**
     * @return the index of the hovered (if any), plus true if start marker, false if end marker.
     * @param event the mouse event.
     */
    std::pair<OptionalInt, bool> getHoveredIndex(const juce::MouseEvent& event) const noexcept;

    /**
     * Updates the location and size of the views. Call this after new Views are added, or if the item size changes.
     * This provokes a Repaint, so that the loop marker is well located.
     */
    void locateAndResizeItems() noexcept;

    /**
     * Updates the internal colors from the look and feel color.
     * @param repaint true to repaint the Component.
     */
    void updateColorsFromLookAndFeel(bool repaint = false) noexcept;

    Listener& listener;
    const int id;                                                   // The ID of the bar.
    int itemWidth;                                                  // How large in pixels is every "cell".
    int endMargin;
    bool showIndexes;                                               // True to show the indexes as text.
    const LookAndFeelConstants::Colors barColorId;                  // Recreate the colors from this look'n'feel color.
    juce::Colour barColor;
    juce::Colour barColorDisabled;
    juce::Colour barColorHovered;
    juce::Colour highlightItemColor;

    DisplayData displayData;                                        // What is displayed.
    OptionalInt hoveredLeftMarkerIndex;                             // If present, the index of the hovered left marker.
    OptionalInt hoveredRightMarkerIndex;                            // If present, the index of the hovered right marker.
    std::vector<int> highlightedIndexes;                            // May be empty.

    std::vector<std::unique_ptr<juce::Label>> labels;               // Filled only if needed.

    juce::Path startMarkerPath;
    juce::Path endMarkerPath;
    juce::Path hoveredStartMarkerPath;
    juce::Path hoveredEndMarkerPath;
    juce::Path horizontalLinePath;
};

}   // namespace arkostracker
