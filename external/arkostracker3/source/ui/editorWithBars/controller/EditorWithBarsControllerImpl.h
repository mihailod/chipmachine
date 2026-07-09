#pragma once

#include <unordered_map>

#include "../view/AreaType.h"
#include "EditorWithBarsController.h"
#include "EditorWithBarsControllerSpecific.h"
#include "helper/VisibilityHandler.h"

namespace arkostracker 
{

class MainController;

/** Partial implementation of the Controller for the Editor With Bars. Specific implementation are done via composition. */
class EditorWithBarsControllerImpl : public EditorWithBarsController
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param specificControllerFactory the factory to instantiate the specific controller.
     * @param defaultCursorAreaType the first area type where the cursor is.
     */
    EditorWithBarsControllerImpl(MainController& mainController, EditorWithBarsControllerSpecific::Factory& specificControllerFactory, AreaType defaultCursorAreaType) noexcept;

    // TypedInstrumentEditorController method implementations.
    // ===================================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void onNewItemSelected(const Id& itemId, bool force) noexcept override;
    void getKeyboardFocus() noexcept override;

    // HeaderController method implementations.
    // ===========================================
    void onUserWantsToSetStartInHeader(int newValue) noexcept override;
    void onUserWantsToSetLoopEndInHeader(int newValue) noexcept override;
    void onUserWantsToSetSpeedInHeader(int newValue) noexcept override;
    void onUserWantsToSetShiftInHeader(int newValue) noexcept override;
    void onUserWantsToSwitchZoomXInHeader() noexcept override;
    void onUserWantsToToggleLoopInHeader(OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToToggleRetrigInHeader() noexcept override;
    void onUserWantsToShrinkRows(bool forceShrinkFull) noexcept override;
    bool showShrinkRows() const noexcept override;

    // BarAreaController method implementations.
    // ============================================
    void onUserClickOnBar(AreaType areaType, int barIndex, int value, const juce::MouseEvent& event, bool isDrag) noexcept override;
    void onUserWantsToSlideValue(AreaType areaType, int barIndex, int value, Speed modifierSpeed, bool increase) noexcept override;
    void onMouseMoveOverBar(AreaType areaType, int barIndex) noexcept override;
    void onMouseExit() noexcept override;
    void onUserWantsToEnlargeArea(AreaType areaType) noexcept override;
    void onUserWantsToShrinkArea(AreaType areaType) noexcept override;
    void onUserWantsToEnlargeArea() noexcept override;
    void onUserWantsToShrinkArea() noexcept override;
    void onUserWantsToToggleHideFromCursor(bool above) noexcept override;
    void onUserWantsToToggleInstrumentRetrig() noexcept override;
    void onUserWantsToSetSpeed(int newSpeed) noexcept override;
    void onUserWantsToSetShift(int newShift) noexcept override;
    void onUserWantsToSwitchXZoom() noexcept override;
    void onUserWantsToSetLoopBarStart(OptionalValue<AreaType> areaType, int newStart) noexcept override;
    void onUserWantsToSetLoopBarEnd(OptionalValue<AreaType> areaType, int newEnd) noexcept override;
    void onUserWantsToToggleLoop(OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToToggleLoopFromCursor(bool isMainLoop) noexcept override;
    void onLeftHeaderHideClicked(AreaType areaType) noexcept override;
    void onUserWantsToSetLoopStartFromCursor(bool isMainLoop) noexcept override;
    void onUserWantsToSetEndFromCursor(bool isMainLoop) noexcept override;
    void onUserWantsToIncreaseSpeed(int step) noexcept override;
    bool isAreaHidden(AreaType areaType) const noexcept override;

    // EditorWithBarsController method implementations.
    // ===================================================
    void onItemCellChanged(const Id& newItemId, int cellIndex, bool mustAlsoRefreshPastIndex) noexcept override;
    void onItemMetadataChanged(const Id& newItemId, unsigned int whatChanged) noexcept override;
    void onSelectedItemChangedMustRefreshUi(const Id& newSelectedItemId) noexcept override;
    OptionalId getCurrentlyShownItemId() const noexcept override;
    BarAreaSize getAreaSize(AreaType areaType) const noexcept override;
    OptionalInt getHoveredBarIndex() const noexcept override;
    BarEditorCursorLocation getBarEditorCursorLocation() const noexcept override;
    void onUserWantsToInsert() noexcept override;
    void onUserWantsToDelete() noexcept override;
    void onUserWantsToToggleRetrig() noexcept override;
    void onUserWantsToGenerateIncreasingVolume() noexcept override;
    void onUserWantsToGenerateDecreasingVolume() noexcept override;
    void onUserWantsToMoveCursorInX(bool left, Speed speed) noexcept override;
    void onUserWantsToMoveCursorInY(bool up, Speed speed) noexcept override;
    void onUserWantsToChangeValue(bool increase, Speed speed) noexcept override;
    void onUserWantsToResetValue() noexcept override;
    void moveCursorLocationTo(AreaType areaType, int barIndex) noexcept override;
    void onUserWantsToEditValue() noexcept override;
    void onUserWantsToDuplicateColumnWhereCursorIs() noexcept override;
    void onUserWantsToDuplicateValueWhereCursorIs() noexcept override;

    juce::Component* getComponentWithTargetCommands() noexcept override;
    void highlightBarIndexes(const std::vector<int>& indexes) noexcept override;

private:
    static const int smallXZoomRate;
    static const int normalXZoomRate;
    static const int largeXZoomRate;
    static const int aheadColumnCount;

    /** The various zoom X steps. */
    enum class ZoomX : uint8_t
    {
        small,
        normal,
    };

    /**
     * Refreshes all the UI. This is called on init, or when switching to another instrument for example.
     * Note: it does NOT handle a new X Zoom.
     * @param ensureCursorVisible true to ensure the cursor is visible. This is not always wanted when other
     * things than bars are modified.
     * @param shrinkRows true to shrink rows. Useful when the UI appears.
     */
    void refreshCompleteUi(bool ensureCursorVisible = true, bool shrinkRows = true) noexcept;
    /**
     * Refreshes the bars a whole index.
     * @param cellIndex the cell index.
     */
    void refreshUiColumn(int cellIndex) const noexcept;

    /**
     * Creates or removes the bar views (if needed), and updates their data if wanted.
     * @param updateFromIndex the index from which to update the data.
     */
    void updateBars(int updateFromIndex = 0) const noexcept;

    /** Updates the non-bar data in the View (header, etc.). */
    void updateTopHeaderView() const noexcept;

    /**
     * @return how many horizontal cells to display. May be larger than what the item contains if ahead columns are wanted!
     * @param withAheadColumns if true, the additional ahead columns are accounted for. If false, only the real size is returned.
     */
    int getDisplayedCellIndexCount(bool withAheadColumns = true) const noexcept;

    /**
     * Checks the given value, and modifies the item internally if it is worth it.
     * This action called should notify the change, thus provoking the UI to update.
     * @param areaType the AreaType of the value change.
     * @param barIndex the index where the change is.
     * @param value the desired value. It may be out of bounds (because of mouse wheel for example).
     */
    void checkValueAndChangeItem(AreaType areaType, int barIndex, int value) const noexcept;

    /** @return true if the given AreaType can be shrunk. */
    bool isShrinkAreaAllowed(AreaType areaType) const noexcept;
    /** @return true if the given AreaType can be enlarged. */
    bool isEnlargeAreaAllowed(AreaType areaType) const noexcept;

    /**
     * Changes the area size. This updates the UI. The validity of such action is not checked here.
     * @param areaType the Area Type.
     * @param enlarge true to enlarge, false to shrink.
     */
    void changeAreaSizeAndUpdate(AreaType areaType, bool enlarge) noexcept;

    /** Updates the internal left headers data, and the auto-spread internal data, and refresh the UI. */
    void updateLeftHeadersAndAutoSpreadUi() noexcept;

    /** Updates the internal left headers data, and the auto-spread internal data. This does not refresh the UI at all. */
    void updateLeftHeadersAndAutoSpreadData() noexcept;
    /**
     * Updates one internal left header data, and the auto-spread internal data. This does not refresh the UI at all.
     * @param areaType the area to update.
     */
    void updateLeftHeaderAndAutoSpreadData(AreaType areaType) noexcept;

    /**
     * Refreshes the UI areas from the internal values (not the bar data themselves, but
     * the headers, etc.).
     * Before calling, updateLeftHeaderData should be called to update the internal data.
     */
    void refreshUiAreasMetadataFromInternalData() const noexcept;

    /**
     * Refreshes the bars of an area.
     * @param areaType the area type to refresh.
     */
    void refreshAreaBars(AreaType areaType) const noexcept;

    /**
     * Called when the user wants to change the start and/or end of the loop. At least one item should be given.
     * @param originalNewStart the new start. Will be corrected.
     * @param originalNewEnd the new end. Will be corrected.
     */
    void onUserWantsToSetLoopStartAndEnd(OptionalInt originalNewStart, OptionalInt originalNewEnd) const noexcept;

    /**
     * Instantiates the View implementation, and fills it. This must be called only when the parent is created, or when changing the whole
     * kind of view (bar editor to "nothing selected" for example).
     * The parent view MUST have been set.
     */
    void instantiateViewAndRefreshUi() noexcept;

    /** Called when an item of the contextual menu is clicked. This is only to manage the hovering state right. */
    void onContextualMenuItemClicked() noexcept;

    /**
     * Removes the possible contextual hovered bar, if present, and if different from the given hovered bar.
     * @param hoveredBarIndex the index of the hovered bar.
     */
    void removeContextualMenuHoveredBarIfPresent(int hoveredBarIndex) noexcept;

    /**
     * @return the bar index that is hovered, and the shown item id. If any problem, the bar index is empty.
     * Asserts if a problem is detected.
     */
    std::pair<OptionalInt, Id> getSafeHoveredBarIndex() const noexcept;

    /**
     * @return the zoom value, from the given ZoomX.
     * @param zoomX the ZoomX.
     */
    static int getXZoomValueFromZoomX(ZoomX zoomX) noexcept;

    /** Makes sure the cursor is not on an hidden row, then scrolls in order to see the cursor. */
    void ensureCursorVisible() noexcept;

    /**
     * Moves the cursor to the given location. Refreshes the UI if needed.
     * @param areaType the area type. Must be valid.
     * @param barIndex the bar index. May be invalid, will be corrected.
     * @param allowAheadColumn true to allow to reach the past-line column. False to reach the last sound column.
     */
    void moveCursorTo(AreaType areaType, int barIndex, bool allowAheadColumn) noexcept;

    /** Corrects the cursor if needed, refreshes the UI if moved, centers on it. */
    void correctCursorIfNeededAndRefreshUi() noexcept;

    /**
     * Corrects the cursor in X if needed. Only the internal value is refreshed, not the UI.
     * @param allowAheadColumn true to allow to reach the past-line column. False to reach the last sound column.
     * @return true if a change has been performed.
     */
    bool correctCursorXInternally(bool allowAheadColumn) noexcept;

    /**
     * Toggles the possible stored area type for the current item. This only changes the internal structure.
     * @return true if the toggle was performed.
     */
    bool toggleStoredHiddenAreaType(AreaType areaType) noexcept;

    /**
     * @return an Area Type that is not hidden, from the given one and a desired direction.
     * @param originalAreaType the area type where the cursor is. It may be hidden.
     * @param down true if we want to go down, false for up.
     */
    AreaType findNonHiddenAreaType(AreaType originalAreaType, bool down) const noexcept;

    /** If needed, moves the cursor to a non-hidden area. Nothing happens if it is not needed. */
    void moveCursorToNonHiddenAreaType() noexcept;

    /**
     * Toggles the "hide" state for a given area. This also refreshes the UI.
     * @param areaType the area type.
     */
    void toggleHiddenArea(AreaType areaType) noexcept;

    BoundedComponent* parentView;                                               // The Parent view where all the Views are put.

    std::unique_ptr<EditorWithBarsControllerSpecific> specificController;
    OptionalId shownItemIdOptional;
    ZoomX xZoom;

    std::unique_ptr<EditorWithBarsView> editorView;                             // The view.
    std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData;    // The left headers data to display.
    std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData;                // The auto spread data.

    OptionalInt currentlyHoveredBarIndex;
    OptionalInt hoveredBarIndexWhereContextualMenuOpened;
    bool isContextualMenuOpened;

    BarEditorCursorLocation currentBarEditorCursorLocation;

    VisibilityHandler visibilityHandler;
};

}   // namespace arkostracker
