#pragma once

#include "../../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

/** Abstract Controller of the Test Area. */
class TestAreaController : public ParentViewLifeCycleAware
{
public:
    /** Called when the user wants to toggle the "use selected arp" state. */
    virtual void onUserWantsToToggleUseSelectedArp() noexcept = 0;
    /** Called when the user wants to toggle the "use selected pitch" state. */
    virtual void onUserWantsToToggleUseSelectedPitch() noexcept = 0;

    /** Called when the user wants to play a sound. */
    virtual void onUserWantsToPlaySound() noexcept = 0;
    /** Called when the user wants to toggle mono/polyphony state. */
    virtual void onUserWantsToToggleMonophony() noexcept = 0;

    /** Called when the user wants to edit mono/polyphony channels. */
    virtual void onUserWantsToEditChannels() noexcept = 0;

    /**
     * Called when the user wants to play a note. The UI has not been modified yet.
     * @param noteNumber the note number.
     */
    virtual void onUserWantsToPlayNote(int noteNumber) noexcept = 0;

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;
};

}   // namespace arkostracker
