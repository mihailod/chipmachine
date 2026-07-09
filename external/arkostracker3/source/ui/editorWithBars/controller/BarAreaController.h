#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../business/model/Speed.h"
#include "../../../utils/OptionalValue.h"
#include "../view/AreaType.h"

namespace arkostracker 
{

/** Abstract controller for the BarArea. */
class BarAreaController
{
public:
    /** Destructor. */
    virtual ~BarAreaController() = default;

    /**
     * Called when the user wanted to set a value in a Bar. The UI has not been modified yet.
     * The controller may want to do something, or ignore or correct the change.
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     * @param value the desired value. May be out of bounds. Must be corrected.
     * @param event the mouse event.
     * @param isDrag true if a drag being performed. False if a click down.
     */
    virtual void onUserClickOnBar(AreaType areaType, int barIndex, int value, const juce::MouseEvent& event, bool isDrag) noexcept = 0;

    /**
     * Called when the user used the mouse wheel to increase/decrease the value of a Bar. The UI has not been modified yet.
     * The controller may want to do something, or ignore or correct the change.
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     * @param value the current value. May be out of bounds. Must be corrected.
     * @param modifierSpeed how fast to change.
     * @param increase true if the mouse wheel is for increase, false if for decrease.
     */
    virtual void onUserWantsToSlideValue(AreaType areaType, int barIndex, int value, Speed modifierSpeed, bool increase) noexcept = 0;

    /**
     * Called when the mouse moves over a bar.
     * @param areaType the area type of the hovered bar.
     * @param barIndex the bar index.
     */
    virtual void onMouseMoveOverBar(AreaType areaType, int barIndex) noexcept = 0;

    /** Called when the mouse exits the area. */
    virtual void onMouseExit() noexcept = 0;

    /**
     * The user wants to enlarge an area. It may be rejected.
     * @param areaType the target area.
     */
    virtual void onUserWantsToEnlargeArea(AreaType areaType) noexcept = 0;

    /**
     * The user wants to shrink an area. It may be rejected.
     * @param areaType the target area.
     */
    virtual void onUserWantsToShrinkArea(AreaType areaType) noexcept = 0;

    /** The user wants to enlarge the current area. It may be rejected. */
    virtual void onUserWantsToEnlargeArea() noexcept = 0;
    /** The user wants to shrink the current area. It may be rejected. */
    virtual void onUserWantsToShrinkArea() noexcept = 0;

    /**
     * The user wants to shrink an area. It may be rejected.
     * @param above true to target the area above the cursor, false for below.
     */
    virtual void onUserWantsToToggleHideFromCursor(bool above) noexcept = 0;

    /**
     * Called when the user wants to set the start of the loop bar.
     * @param areaType the area this loop bar is linked to, or absent meaning the main loop.
     * @param newStart the new start.
     */
    virtual void onUserWantsToSetLoopBarStart(OptionalValue<AreaType> areaType, int newStart) noexcept = 0;

    /**
     * Called when the user wants to set the start of a loop bar, from where the cursor is.
     * @param isMainLoop true to change the main loop, false according to the cursor area type (auto-spread).
     */
    virtual void onUserWantsToSetLoopStartFromCursor(bool isMainLoop) noexcept = 0;

    /**
     * Called when the user wants to set the end of a loop bar, from where the cursor is.
     * @param isMainLoop true to change the main loop, false according to the cursor area type (auto-spread).
     */
    virtual void onUserWantsToSetEndFromCursor(bool isMainLoop) noexcept = 0;

    /**
     * Called when the user wants to set the end of the loop bar.
     * @param areaType the area this loop bar is linked to, or absent meaning the main loop.
     * @param newEnd the new end.
     */
    virtual void onUserWantsToSetLoopBarEnd(OptionalValue<AreaType> areaType, int newEnd) noexcept = 0;

    /**
     * Called when the user wants to toggle the state of the loop bar.
     * @param areaType the area this loop bar is linked to, or absent meaning the main loop.
     */
    virtual void onUserWantsToToggleLoop(OptionalValue<AreaType> areaType) noexcept = 0;

    /**
     * Called when the user wants to toggle the state of the loop bar.
     * @param isMainLoop true to change the main loop, false according to the cursor area type (auto-spread).
     */
    virtual void onUserWantsToToggleLoopFromCursor(bool isMainLoop) noexcept = 0;

    /**
     * Called when the hide/show button has been clicked.
     * @param areaType the area type of this header.
     */
    virtual void onLeftHeaderHideClicked(AreaType areaType) noexcept = 0;

    /**
     * Called when the user wants to set the speed of the item.
     * @param newSpeed the new speed.
     */
    virtual void onUserWantsToSetSpeed(int newSpeed) noexcept = 0;

    /**
     * Called when the user wants to set the shift of the item.
     * @param newShift the new shift.
     */
    virtual void onUserWantsToSetShift(int newShift) noexcept = 0;

    /** Called when the user wants to toggle the state of the instrument retrig. */
    virtual void onUserWantsToToggleInstrumentRetrig() noexcept = 0;

    /** Called when the user wants to change the X zoom. */
    virtual void onUserWantsToSwitchXZoom() noexcept = 0;

    /**
     * Called when the user wants to increase the speed.
     * @param step how much to increase (maybe negative).
     */
    virtual void onUserWantsToIncreaseSpeed(int step) noexcept = 0;

    /** @return true if the rows can be hidden. Expressions shouldn't, not useful. */
    virtual bool canHideRows() const noexcept = 0;

    /** @return true if a specific row can be hidden. */
    virtual bool canHideRow(AreaType areaType) const noexcept = 0;

    /**
     * @return true if the area is hidden. If not found, considered true.
     * @param areaType the area type to find.
     */
    virtual bool isAreaHidden(AreaType areaType) const noexcept = 0;
};

}   // namespace arkostracker
