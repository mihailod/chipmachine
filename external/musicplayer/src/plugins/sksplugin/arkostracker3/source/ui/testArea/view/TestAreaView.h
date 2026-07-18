#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Abstract View of the Test Area. */
class TestAreaView : public juce::Component
{
public:
    /** Holds the data to display. */
    class DisplayedData
    {
    public:
        DisplayedData(const bool pUseSelectedArp, const bool pUseSelectedPitch, const bool pUseMono, const OptionalInt pNoteToShow, const OptionalInt pPendingNoteToShow,
            const int pUsedArpeggioIndex, const int pUsedPitchIndex) :
                useSelectedArp(pUseSelectedArp),
                useSelectedPitch(pUseSelectedPitch),
                useMono(pUseMono),
                noteToShow(pNoteToShow),
                pendingNoteToShow(pPendingNoteToShow),
                usedArpeggioIndex(pUsedArpeggioIndex),
                usedPitchIndex(pUsedPitchIndex)
        {
        }

        const bool useSelectedArp;
        const bool useSelectedPitch;
        const bool useMono;
        const OptionalInt noteToShow;
        const OptionalInt pendingNoteToShow;
        const int usedArpeggioIndex;
        const int usedPitchIndex;
    };

    /**
     * Sets the data to display. Only the changed information are updated.
     * @param displayedData the data to display.
     */
    virtual void setDisplayedData(const DisplayedData& displayedData) noexcept = 0;
};

}   // namespace arkostracker
