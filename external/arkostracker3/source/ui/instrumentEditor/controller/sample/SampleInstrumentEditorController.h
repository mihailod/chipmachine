#pragma once

#include "../ItemEditorController.h"
#include "../../../../utils/OptionalValue.h"

namespace arkostracker
{

/** Controller of the Sample Instrument Editor. */
class SampleInstrumentEditorController : public ItemEditorController
{
public:
    /**
     * Called when the user changed the multiplier value. The value may need correction, and the data and UI refreshed.
     * @param multiplier the new value.
     */
    virtual void onUserantsToChangedMultiplier(double multiplier) noexcept = 0;

    /**
     * Called when the user changed the loop value. The value may need correction, and the data and UI refreshed.
     * @param newLoopStart the possible new loop start.
     * @param newEnd the possible new end.
     * @param newIsLooping the possible new loop state.
     */
    virtual void onUserWantsToSetNewLoop(OptionalInt newLoopStart, OptionalInt newEnd, OptionalBool newIsLooping) noexcept = 0;

    /**
     * Called when the user wants to zoom in or out.
     * @param zoomIn true to zoom in, false to zoom out.
     * @param clickedOffset the clicked offset in the sample.
     */
    virtual void onUserWantsToZoom(bool zoomIn, int clickedOffset) noexcept = 0;

    /**
     * Called when the user wants to scroll.
     * @param right true to go right, false to go left.
     */
    virtual void onUserWantsToScroll(bool right) noexcept = 0;

    /**
     * Called when the user wants to scroll to a specific position.
     * @param newStartOffset the new offset.
     */
    virtual void onUserWantsToScrollTo(int newStartOffset) noexcept = 0;

    /**
     * Called when the user wants to change the diginote.
     * @param newDigiNote the new diginote.
     */
    virtual void onUserWantsToChangeDigiNote(int newDigiNote) noexcept = 0;

    /**
     * Called when the user wants to change the sample itself.
     * @param fileToLoad the WAV file to load. May be inexistant.
     */
    virtual void onUserWantsToChangeSampleFile(const juce::File& fileToLoad) noexcept = 0;
};

}   // namespace arkostracker
