#pragma once

#include <memory>
#include <unordered_map>

#include "../../../song/instrument/psg/PsgSection.h"
#include "../../instrumentEditor/controller/psg/BarDataCreator.h"
#include "../view/EditorWithBarsView.h"

namespace arkostracker 
{

class MainController;
class EditorWithBarsController;

/** Abstract specialized Controller. */
class EditorWithBarsControllerSpecific
{
public:
    /** A factory to instantiate this specific Controller. */
    class Factory
    {
    public:
        /** Destructor. */
        virtual ~Factory() = default;

        /**
         * @return a instance of the specific controller.
         * @param mainController the Main Controller, as it is handy.
         * @param editorController the Editor controller which will use this specific controller.
         */
        virtual std::unique_ptr<EditorWithBarsControllerSpecific> buildSpecificController(MainController& mainController, EditorWithBarsController& editorController) noexcept = 0;
    };

    /** Destructor. */
    virtual ~EditorWithBarsControllerSpecific() = default;

    /** @return the id of the currently selected item, if any. */
    virtual OptionalId getCurrentItemId() const noexcept = 0;

    /**
     * Creates a View instance.
     * @param editorController the controller.
     * @param xZoomRate the default x Zoom rate/
     * @param selectedItemId the id of the selected item. May be absent if nothing is selected.
     * @return the view.
     */
    virtual std::unique_ptr<EditorWithBarsView> createView(EditorWithBarsController& editorController, int xZoomRate, const OptionalId& selectedItemId) noexcept = 0;

    /**
     * @return true if the View instance must be recreated. This can be needed when switching from a normal editor view to a "none selected" for example.
     * @param currentItemId the possible current item id.
     * @param newItemId the new item id.
     */
    virtual bool shouldViewInstanceRecreated(const OptionalId& currentItemId, const Id& newItemId) const noexcept = 0;

    /**
     * @return the data that are not cells.
     * @param itemId the id of the item.
     */
    virtual EditorWithBarsView::DisplayTopHeaderData getDisplayedTopHeaderData(const Id& itemId) const noexcept = 0;

    /**
     * @return the length of the item.
     * @param itemId the id of the item.
     */
    virtual int getItemLength(const Id& itemId) const noexcept = 0;

    /**
     * Creates the Bar Data for one whole column. This is more efficient than using createBarData method, as the lock on the Cell
     * is done once.
     * @param itemId the id of the item.
     * @param barIndex the index.
     * @return the AreaType linked to their Bar Data.
     */
    virtual std::unordered_map<AreaType, BarAndCaptionData> createBarsData(const Id& itemId, int barIndex) const noexcept = 0;

    /**
     * Checks the given value, and modifies the item internally if it is worth it. The end may be updated.
     * This action called should notify the change, thus provoking the UI to update.
     * @param itemId the id of the item.
     * @param areaType the AreaType of the value change.
     * @param barIndex the index where the change is.
     * @param value the desired value. It may be out of bounds (because of mouse wheel for example).
     */
    virtual void checkValueAndChangeItem(const Id& itemId, AreaType areaType, int barIndex, int value) noexcept = 0;

    /** @return all the area types, in no specific order. */
    virtual std::unordered_set<AreaType> getAreaTypes() const noexcept = 0;

    /** @return the size of all area types. */
    virtual std::unordered_map<AreaType, BarAreaSize> getAreaTypesToInitialSize() const noexcept = 0;
    /** @return the size of all area types. */
    virtual std::unordered_map<AreaType, BarAreaSize> getAreaTypesToMaximumSize() const noexcept = 0;

    /** @return true if auto spread is allowed (for all the AreaTypes). */
    virtual bool isAutoSpreadAllowed() const noexcept = 0;

    /**
     * @return the auto-spread loop. Their allowance must have been tested before.
     * @param itemId the id of the item. If not existing, asserts but returns a dummy value.
     * @param areaType the area type. Must be valid.
     */
    virtual Loop getAutoSpreadLoop(const Id& itemId, AreaType areaType) const noexcept = 0;

    /**
     * @return the length of the item, and its main loop.
     * @param itemId the id of the item. If not existing, asserts but returns a dummy value.
     */
    virtual std::pair<int, Loop> getItemLengthAndLoop(const Id& itemId) const noexcept = 0;

    /**
     * Sets the start/end of the main loop of an item. The validity must have been checked before. This should notify the change somehow.
     * @param itemId the id of the item.
     * @param newStart the possible new start.
     * @param newEnd the possible new end.
     * @param newSpeed the possible new speed.
     * @param newShift the possible new shift, if relevant.
     * @param newIsLoop the possible new loop.
     * @param newIsRetrig the possible new retrig, if relevant.
     * @param modifiedSectionToAutoSpreadLoop the possible new auto-spread loops, if relevant.
     */
    virtual void setItemMetadata(const Id& itemId, OptionalInt newStart, OptionalInt newEnd, OptionalInt newSpeed, OptionalInt newShift, OptionalBool newIsLoop,
                                 OptionalBool newIsRetrig, std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) noexcept = 0;

    /**
     * Called when the item metadata has changed (the item index validity has been checked).
     * @param whatChanged the flag about what has changed. The implementation should know what to do with that according to the observer it registers to.
     * @return true if the change is interesting (the UI should be updated).
     */
    virtual bool isItemMetadataChangeFlagsInteresting(unsigned int whatChanged) noexcept = 0;

    /**
     * Called when the user wants to toggle the state of the loop bar.
     * @param itemId the id of the item.
     * @param areaType the area this loop bar is linked to, or absent meaning the main loop.
     */
    virtual void onUserWantsToToggleLoop(const Id& itemId, OptionalValue<AreaType> areaType) noexcept = 0;

    /**
     * Called when the user wants to increase the speed.
     * @param itemId the id of the item.
     * @param step how much to increase (maybe negative).
     */
    virtual void onUserWantsToIncreaseSpeed(const Id& itemId, int step) noexcept = 0;

    /**
     * Called when the user wants to toggle the state of the Instrument retrig.
     * @param itemId the id of the item.
     */
    virtual void onUserWantsToToggleInstrumentRetrig(const Id& itemId) noexcept = 0;

    /**
     * Called when the user wants to change the start and/or end of an auto-spread. At least one item should be given.
     * @param itemId the id of the item.
     * @param areaType the area type where the auto-spread is.
     * @param newStart the new start. Will be corrected.
     * @param newEnd the new end. Will be corrected.
     */
    virtual void onUserWantsToSetAutoSpreadStartAndEnd(const Id& itemId, AreaType areaType, OptionalInt newStart, OptionalInt newEnd) noexcept = 0;

    /**
     * @return the speed of the item.
     * @param itemId the id of the item. If invalid, asserts and returns a dummy value.
     */
    virtual int getItemSpeed(const Id& itemId) noexcept = 0;

    /**
     * @return the shift of the item.
     * @param itemId the id of the item. If invalid, asserts and returns a dummy value.
     */
    virtual int getItemShift(const Id& itemId) noexcept = 0;

    /** @return the corrected speed from the given one. May already be valid. */
    virtual int correctItemSpeed(int speed) const noexcept = 0;
    /** @return the corrected shift from the given one. May already be valid. */
    virtual int correctItemShift(int shift) const noexcept = 0;

    /**
     * @return the speeds to use when sliding the value (using the mouse wheel for example).
     * Can return an empty map if there is no specific value to use. Each entry must have 3 speeds.
     */
    virtual const std::unordered_map<AreaType, std::vector<int>>& getSpecificAreaTypeToSlideSpeed() const noexcept = 0;

    /**
     * Duplicates the column, if possible.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index. May be invalid (out of bounds).
     * @return true if the action was performed.
     */
    virtual bool duplicateColumn(const Id& itemId, int cellIndex) noexcept = 0;

    /**
     * Duplicates the value, if possible.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index. May be invalid (out of bounds).
     * @param areaType where the value is.
     * @return true if the action was performed.
     */
    virtual bool duplicateValue(const Id& itemId, int cellIndex, AreaType areaType) noexcept = 0;

    /**
     * Deletes the column, if possible.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index. May be invalid (out of bounds).
     * @return true if it could be done.
     */
    virtual bool deleteColumn(const Id& itemId, int cellIndex) noexcept = 0;

    /**
     * Toggles the retrig, if possible.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index. May be invalid (out of bounds).
     */
    virtual void toggleRetrig(const Id& itemId, int cellIndex) noexcept = 0;

    /**
     * Generates an increasing volume curve.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index where to start. May be invalid (out of bounds).
     */
    virtual void generateIncreasingVolume(const Id& itemId, int cellIndex) noexcept = 0;

    /**
     * Generates a decreasing volume curve.
     * @param itemId the ID of the item (instrument, etc.).
     * @param cellIndex the cell index. May be invalid (out of bounds).
     */
    virtual void generateDecreasingVolume(const Id& itemId, int cellIndex) noexcept = 0;

    /**
     * Called to open a contextual menu. The implementation may not do anything.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the area where the menu is opened.
     * @param cellIndex the cell index. May be invalid (out of bounds).
     * @param onMenuItemClicked function called when the menu item is clicked (meaning the menu can be closed).
     */
    virtual void openContextualMenu(const Id& itemId, AreaType areaType, int cellIndex, std::function<void()> onMenuItemClicked) noexcept = 0;

    /**
     * Called when the user wants to edit a value on a Bar. The controller may want to do something, or ignore this event.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     */
    virtual void onUserWantsToEditValue(const Id& itemId, AreaType areaType, int barIndex) noexcept = 0;

    /**
     * Called when the user wants to edit a value on a Bar. The controller may want to do something, or ignore this event.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     */
    virtual void onUserWantsToResetValue(const Id& itemId, AreaType areaType, int barIndex) noexcept = 0;

    /**
     * @return true if the area of the item is full of default values, and thus, can be hidden.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     */
    virtual bool doesContainOnlyDefaultData(const Id& itemId, AreaType areaType) noexcept = 0;

    /**
     * @return the minimum size for an area, according to its data.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     * @param forceShrinkFull false to shrink "normally". True to shrink even to the smallest height. Most rows will ignore this.
     */
    virtual BarAreaSize getMinimumSizeForData(const Id& itemId, AreaType areaType, bool forceShrinkFull) noexcept = 0;

    /**
     * @return true if the row can be hidden.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     */
    virtual bool canHideRow(const Id& itemId, AreaType areaType) noexcept = 0;
};

}   // namespace arkostracker
