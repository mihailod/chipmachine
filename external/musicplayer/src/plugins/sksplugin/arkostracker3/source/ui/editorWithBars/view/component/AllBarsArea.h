#pragma once

#include <unordered_map>

#include "../../../../business/model/Loop.h"
#include "../../../../utils/WithParent.h"
#include "../../../components/LoopBar.h"
#include "../../../components/ViewPortWithHorizontalMouseWheel.h"
#include "BarArea.h"
#include "BarAreaHolder.h"
#include "BarAreaSize.h"
#include "LeftHeader.h"
#include "Section.h"

namespace arkostracker 
{

class AllBarsAreaDataProvider;
class BarAreaController;

/**
 * Holds the BarAreas and their left header. It does not contain the header.
 * Uses delegation to specialize its data.
 *
 * Call initViews first.
 */
class AllBarsArea final : public LoopBar::Listener,
                          public LeftHeader::Listener
{
public:
    /**
     * Constructor.
     * @param dataProvider object that knows what to build.
     * @param barAreaController the controller to the BarArea events.
     * @param xZoomRate an arbitrary value for the X zoom (>=0).
     */
    AllBarsArea(AllBarsAreaDataProvider& dataProvider, BarAreaController& barAreaController, int xZoomRate) noexcept;

    /**
     * Initializes the Views. They are instantiated and added to the parent, but their size is not set.
     * This method must be called only once.
     * @param parent where to add the Views.
     */
    void initViews(juce::Component& parent) noexcept;

    /** @return the component to focus on (to grab focus). */
    juce::Component* getFocusableComponent() noexcept;

    /**
     * Locates the Views. Call this at every resize or when actions requires them to move, shrink, etc.
     * @param x the X.
     * @param y the Y.
     * @param width the available width.
     * @param height the available height.
     */
    void locateViews(int x, int y, int width, int height) noexcept;

    /** Locates all the Views. This uses the previously stored base X/Y and size stored when the other locateViews method is called. */
    void locateViews() noexcept;

    /**
     * Sets the loop bar display data. Updates the UI if there is a change.
     * @param loopBarData the data.
     */
    void setLoopBarDisplayData(const LoopBar::DisplayData& loopBarData) noexcept;

    /**
     * Sets the bar count. Bars may be removed, or created. Created bars will have default empty values.
     * @param barCount how many bars.
     */
    void setDisplayedBarCount(int barCount) noexcept;

    /**
     * Sets the data to a Bar. This refreshes the UI. Nothing happens if the data is the same.
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     * @param barData the data of the bar to display.
     * @param captionData the data of the Caption.
     */
    void showBarData(AreaType areaType, int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) const noexcept;

    /**
     * Sets new area sizes. If different, values are stored. This does not refresh the UI.
     * @param areaTypeToSize the new sizes.
     * @return true if a change has been done.
     */
    bool setAreaSizesData(std::unordered_map<AreaType, BarAreaSize> areaTypeToSize) noexcept;

    /**
     * Sets new area auto spread data. If different, values are stored. This does not refresh the UI.
     * @param areaTypeToAutoSpreadData the new auto spread data.
     * @return true if a change has been done.
     */
    bool setAreaAutoSpreadData(std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData) noexcept;

    /**
     * Sets new area left header display data. If different, values are stored. This does not refresh the UI.
     * @param areaTypeToLeftHeaderDisplayData the new left header display data.
     * @return true if a change has been done.
     */
    bool setLeftHeaderDisplayData(std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData) noexcept;

    /**
     * Sets the X zoom rate, refreshing the UI.
     * @param newXZoomRate the X zoom rate.
     */
    void setXZoomRate(int newXZoomRate) noexcept;

    /** Updates the auto-spread loop views, from the internal data. */
    void refreshAutoSpreadViewsFromInternalData() noexcept;

    /**
     * Refreshes all the left headers, according to the internal data map. Nothing happens if there are no changes.
     * @param forceRefresh true to force the refresh.
     */
    void refreshLeftHeaders(bool forceRefresh = false) noexcept;

    /**
     * @return the value held by a bar. If not found, 0 may be returned.
     * @param areaType the area type. Must be valid.
     * @param barIndex the index of the bar. May be invalid.
     */
    int getValue(AreaType areaType, int barIndex) const noexcept;

    /**
     * @return a next/previous area type. May be the same if not found, or out of bounds.
     * @param areaType the current area type. If not found, the same area type is returned (asserts).
     * @param offset positive to go "down". May be out of bounds, will be corrected.
     */
    AreaType getShiftedAreaType(AreaType areaType, int offset) const noexcept;

    /**
     * Scrolls in order to see the given position.
     * @param areaType the area type. Must be valid.
     * @param barIndex the index of the bar. May be invalid.
     */
    void ensureVisible(AreaType areaType, int barIndex) noexcept;

    /**
     * Highlights indexes. Typically used by instrument being played.
     * @param indexes the indexes. May be empty.
     */
    void highlightBarIndexes(const std::vector<int>& indexes) noexcept;

    // LoopBar::Listener method implementations.
    // ============================================
    void onUserWantsToSetLoopBarStart(int id, int newStart) noexcept override;
    void onUserWantsToSetLoopBarEnd(int id, int newEnd) noexcept override;
    void onUserWantsToToggleLoop(int id) noexcept override;

    // LeftHeader::Listener method implementations.
    // ===============================================
    void onLeftHeaderPlusClicked(AreaType areaType) noexcept override;
    void onLeftHeaderMinusClicked(AreaType areaType) noexcept override;
    void onLeftHeaderAutoSpreadClicked(AreaType areaType) noexcept override;
    void onLeftHeaderHideClicked(AreaType areaType) noexcept override;

private:
    static const int mainLoopBarId;

    static const int heightCountPerArea;
    static const int leftViewportWidth;
    static const int postSectionMargin;

    static const int leftHeaderHeightHidden;

    /** A custom Viewport to intercept some events, and manage the "horizontal" mouse wheel via Alt+wheel. */
    class CustomViewport final : public ViewPortWithHorizontalMouseWheel,
                                 WithParent<AllBarsArea>
    {
    public:
        explicit CustomViewport(AllBarsArea& parent) :
                WithParent(parent)
        {
        }

        void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
        {
            parentObject.onScrollingOnBarViewport(newVisibleArea);
        }
    };

    // ==============================================================

    /** @return the width of each bar, according to the current zoom factor. */
    int getItemWidth() const noexcept;

    /**
     * Called when a Bar Viewport scrolling has been done.
     * @param newVisibleArea the visible area of the Bar Viewport.
     */
    void onScrollingOnBarViewport(const juce::Rectangle<int>& newVisibleArea) noexcept;

    /**
     * Scrolls on both Viewports.
     * @param x the X. Only the bar Viewport is using it.
     * @param y the Y.
     */
    void scrollOnViewports(int x, int y) noexcept;

    /** @return the width of the areas, according to the current zoom factor, and how many items there are. */
    int calculateAreasWidth() const noexcept;
    /**
     * @return the height, in pixels, of an Area, according to the height given by the client.
     * @param areaType the area type.
     */
    int getAreaHeight(AreaType areaType) const noexcept;

    /**
     * @return the BarArea from the given AreaType. It MUST exist!
     * @param areaType the area type.
     */
    BarArea& getBarArea(AreaType areaType) const noexcept;

    /**
     * @return the AreaType from the given bar id. Absent if matching the main loop.
     * @param barId the bar ID, which is actually directly converted into an AreaType.
     */
    static OptionalValue<AreaType> getAreaTypeFromBarId(int barId) noexcept;

    /**
     * Tries to locate the section for the given AreaType, if it exists. This only locates them in the Viewport.
     * Nothing happens if the Area doesn't have a section.
     * @param x the x of the section.
     * @param y the Y where they should be. Will be increased if the section is shown.
     * @param width the width of the section.
     * @param areaType the current AreaType where the section may be displayed.
     */
    void addSection(int x, int& y, int width, AreaType areaType) noexcept;

    void displayVerticalLoopBars() noexcept;

    AllBarsAreaDataProvider& dataProvider;
    BarAreaController& barAreaController;

    int xZoomRate;                                      // An arbitrary value for the X zoom (>=0).
    int barCount;                                       // How many bars there are.
    int globalStartX;                                   // Where to start the display in X.
    int globalStartY;                                   // Where to start the display in Y.
    int globalTotalWidth;                               // The width to use when displaying all the views.
    int globalTotalHeight;                              // The height to use when displaying all the views.

    Loop cachedMainLoop;                                // Stored because the vertical loop bars display is delayed.

    CustomViewport leftViewport;                        // Viewport on the left, synchronized vertically with the bar viewport.
    juce::Component leftViewportComponent;              // The direct child of the left Viewport.

    CustomViewport barViewport;                         // Where the Bars are put.
    BarAreaHolder barViewportComponent;                 // The direct child of the Viewport, where all the bars are put.

    LoopBar mainLoopBar;                                // The main loop bar (that is, not the auto-spread loop).
    std::vector<std::unique_ptr<BarArea>> barAreas;     // Holds the bar areas, in order.
    std::unordered_map<AreaType, std::unique_ptr<LoopBar>> areaTypeToAutoSpreadBar;     // The auto-spread bar views.

    std::vector<AreaType> allAreaTypes;                 // All the area types that are seen.

    std::unordered_map<AreaType, BarAreaSize> areaTypeToCurrentSize;                        // The current size of every AreaType.
    std::unordered_map<AreaType, std::unique_ptr<LeftHeader>> areaTypeToLeftHeader;         // The LeftHeader for each area.
    std::unordered_map<AreaType, std::vector<int>> areaTypeToHeights;                       // The possible height (in pixels) an Area can reach, for each BarAreaSize.
    std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData;    // The left headers data to display.
    std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData;                            // The auto spread data.

    std::unordered_map<AreaType, std::unique_ptr<Section>> areaTypeToSection;               // Section Views, may be displayed before an area. Only a few entries at best.
};

}   // namespace arkostracker
