#pragma once

#include "../../../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

/** Abstract controller of the Main Toolbar. */
class MainToolbarController : public ParentViewLifeCycleAware
{
public:
    /** Called when the timer is clicked. */
    virtual void onTimerClicked() noexcept = 0;

    /**
     * Called when the user wants to recall an arrangement.
     * @param arrangementNumber the number of the arrangement.
     */
    virtual void onUserWantsToRestoreArrangement(int arrangementNumber) = 0;

    /** Called when the user wants to play the Song from Start. */
    virtual void onUserWantsToPlaySongFromStart() noexcept = 0;
    /** Called when the user wants to play the Song (at the beginning of current pattern). */
    virtual void onUserWantsToPlaySong() noexcept = 0;
    /** Called when the user wants to play the Pattern from Start. */
    virtual void onUserWantsToPlayPatternFromStart() noexcept = 0;
    /** Called when the user wants to play the Pattern. */
    virtual void onUserWantsToPlayPattern() noexcept = 0;
    /** Called when the user wants to stop the playing. */
    virtual void onUserWantsToStopPlay() noexcept = 0;

    /** Called when the user wants to start/stop the serial communication. */
    virtual void onUserWantsToStartStopSerial() noexcept = 0;

    /**
     * Called when the user wants to set the octave.
     * @param desiredOctave the desired octave, maybe invalid.
     */
    virtual void onUserWantsToSetOctave(int desiredOctave) noexcept = 0;
};

}   // namespace arkostracker

