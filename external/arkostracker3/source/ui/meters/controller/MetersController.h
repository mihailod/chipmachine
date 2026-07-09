#pragma once

namespace arkostracker
{

/** The MetersController holds the meter views, and interacts with the other controllers. */
class MetersController
{
public:
    /** Destructor. */
    virtual ~MetersController() = default;

    /**
     * Calculates and applies the size and location of all the already created views.
     * @param startX the X where to start drawing.
     * @param startY the Y where to start drawing.
     * @param width the width upon which to draw.
     * @param height the height upon which to draw.
     */
    virtual void updateViewLocations(int startX, int startY, int width, int height) = 0;

    /**
     * The user wants to change the mute state.
     * @param channelIndex the channel which state to change.
     * @param newMuteState true if the channel must be muted.
     */
    virtual void onWantToChangeMuteState(int channelIndex, bool newMuteState) = 0;

    /**
     * The user wants to must all except one channel. But if others are all muted, unmute all.
     * @param channelIndex the target channel index.
     */
    virtual void onWantToAllMuteExcept(int channelIndex) = 0;

    /** Switches to the other view. */
    virtual void onUserWantsToSwitchView() noexcept = 0;
};

}   // namespace arkostracker
