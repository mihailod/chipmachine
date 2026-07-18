#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/WithParent.h"
#include "../../components/LoopBar.h"
#include "../../components/ThemedColoredImage.h"
#include "MarkerView.h"
#include "PatternItemView.h"
#include "PlayLineView.h"

namespace arkostracker 
{

/**
 * Holds the linker items. It should be put in a Viewport.
 * This Component defines its own size.
 */
class ItemsHolder final : public juce::Component,
                          public juce::DragAndDropContainer,
                          public juce::DragAndDropTarget            // Only to manage scrolling.
{
public:
    class ScrollListener
    {
    public:
        /** Destructor. */
        virtual ~ScrollListener() = default;

        /**
         * Called when a d'n'd is performed. Maybe the edge of the container is reached, and it should scroll.
         * @param mouseX the mouse X position, according to the top-left of the container.
         */
        virtual void manageScrollingIfNeeded(int mouseX) = 0;
    };

    /**
     * Constructor.
     * @param scrollListener listener to the need of scrolling (to be transmitted to the ViewPort).
     * @param patternItemListener the listener to the events of the positions.
     * @param loopBarListener listens to the events of the LoopBar.
     * */
    ItemsHolder(ScrollListener& scrollListener, PatternItemView::Listener& patternItemListener, LoopBar::Listener& loopBarListener) noexcept;

    /**
     * Sets the data to show.
     * @param patternData the pattern data.
     * @param loopData the loop data.
     * @param playLineData the "play line" data.
     * @param positionToMarker the positions linked to their markers.
     */
    void setDisplayedData(const std::vector<PatternItemView::DisplayedData>& patternData, const LoopBar::DisplayData& loopData,
                          const PlayLineView::DisplayedData& playLineData,
                          const std::unordered_map<int, juce::String>& positionToMarker) noexcept;

    /**
     * @return the X and width of the item, to allow scrolling for example.
     * @param position the position.
     */
    std::pair<int, int> getItemXAndWidth(int position) const noexcept;

    // juce::DragAndDropTarget method implementations.
    // =================================================
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;

private:
    static const int markerY;
    static const int positionNumberY;
    static const int positionNumberHeight;
    static const int loopBarHeight;
    static const int patternItemY;
    static const int patternItemWidth;
    static const int patternItemHeight;
    static const int separatorWidth;
    static const int itemWidth;                                 // Includes the pattern item and the possible spacing.
    static const int separatorBetweenItemsAndCursorLine;

    /**
     * Called when the mouse is hovering.
     * @param event the event.
     */
    void onMouseHovered(const juce::MouseEvent& event) noexcept;
    /** Called when the mouse exited the Component. */
    void onMouseExited() noexcept;

    /** Hides the hovered images, and clears the inner position data. */
    void hideHoveredImages() noexcept;

    /** Needed to know the position of the hovering, else the children gets the events, but not the parent. */
    class InnerMouseListener final : public juce::MouseListener,
                                     public WithParent<ItemsHolder>
    {
    public:
        explicit InnerMouseListener(ItemsHolder& pParent) :
                WithParent(pParent)
        {
        }

        void mouseMove(const juce::MouseEvent& event) override
        {
            const auto newEvent = event.getEventRelativeTo(&parentObject);
            parentObject.onMouseHovered(newEvent);
        }

        void mouseExit(const juce::MouseEvent& /*event*/) override
        {
            parentObject.onMouseExited();
        }
    };

    /**
     * Displays the markers.
     * @param positionToMarker a map linking a position to its marker.
     */
    void displayMarkers(const std::unordered_map<int, juce::String>& positionToMarker) noexcept;

    /**
     * Called when the Add New icon is clicked.
     * @param isLeftButtonClicked true if the left button is clicked.
     */
    void onAddNewPositionClicked(bool isLeftButtonClicked) const noexcept;

    /**
     * Called when the Repeat icon is clicked.
     * @param isLeftButtonClicked true if the left button is clicked.
     */
    void onRepeatPositionClicked(bool isLeftButtonClicked) const noexcept;

    const int horizontalMargin;

    InnerMouseListener mouseListener;

    ScrollListener& scrollListener;
    PatternItemView::Listener& patternItemListener;

    std::vector<std::unique_ptr<PatternItemView>> patternItemViews;
    LoopBar loopBar;
    PlayLineView playLineView;

    std::unordered_map<int, juce::String> cachedPositionToMarker;               // For optimization.
    std::unordered_map<int, std::unique_ptr<MarkerView>> positionToMarkerView;

    ThemedColoredImage addNewPositionImage;
    ThemedColoredImage addRepeatPositionImage;
    OptionalInt hoveredPositionIndex;
};

}   // namespace arkostracker
