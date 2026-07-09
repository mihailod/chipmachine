#pragma once

#include "../../../utils/OptionalValue.h"
#include "../view/AreaType.h"

namespace arkostracker 
{

/** Abstract controller of the header. */
class HeaderController
{
public:
    /** Destructor. */
    virtual ~HeaderController() = default;

    /**
     * Called when the user wants to toggle the state of a loop bar.
     * @param areaType the area this loop bar is linked to, or absent meaning the main loop.
     */
    virtual void onUserWantsToToggleLoopInHeader(OptionalValue<AreaType> areaType) noexcept = 0;

    /** Called when the user wants to toggle the state of the retrig. */
    virtual void onUserWantsToToggleRetrigInHeader() noexcept = 0;

    /**
     * Called when the user wants to set a new start. The controller may ignore this, correct the value, and update the UI.
     * @param newValue the desired value.
     */
    virtual void onUserWantsToSetStartInHeader(int newValue) noexcept = 0;

    /**
     * Called when the user wants to set a new loop end. The controller may ignore this, correct the value, and update the UI.
     * @param newValue the desired value.
     */
    virtual void onUserWantsToSetLoopEndInHeader(int newValue) noexcept = 0;

    /**
     * Called when the user wants to set a new speed. The controller may ignore this, correct the value, and update the UI.
     * @param newValue the desired value.
     */
    virtual void onUserWantsToSetSpeedInHeader(int newValue) noexcept = 0;

    /**
     * Called when the user wants to set a new shift. The controller may ignore this, correct the value, and update the UI.
     * @param newValue the desired value.
     */
    virtual void onUserWantsToSetShiftInHeader(int newValue) noexcept = 0;

    /** Called when the user wants to switch to the next zoom X. The controller may ignore this, correct the value, and update the UI. */
    virtual void onUserWantsToSwitchZoomXInHeader() noexcept = 0;

    /**
     * The user wants to shrink the rows.
     * @param forceShrinkFull false to shrink "normally". True to shrink to minimum height on some rows.
     */
    virtual void onUserWantsToShrinkRows(bool forceShrinkFull) noexcept = 0;

    /** @return true if the Shift must be shown. */
    virtual bool showShift() const noexcept = 0;
    /** @return true if the Loop? must be shown. */
    virtual bool showIsLooping() const noexcept = 0;
    /** @return true if the Retrig? must be shown. */
    virtual bool showIsRetrig() const noexcept = 0;
    /** @return true if the Shrink Rows button must be shown. */
    virtual bool showShrinkRows() const noexcept = 0;
};

}   // namespace arkostracker

