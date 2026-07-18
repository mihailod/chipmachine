#pragma once

#include "../serial/SerialAccess.h"
#include "../serial/SerialController.h"

namespace arkostracker
{

/** Observer to general changes (i.e. data that are not linked to the song itself). */
class GeneralDataObserver
{
public:
    /** Flags about what has changed. */
    enum What
    {
        /** A layout has been restored. */
        layoutRestore = 1U << 0U,
        octave = 1U << 1U,
        /** Tried to stop playing, whereas the song was already stopped. Useful for some special command (dismiss block selection for example). */
        doubleStopPlay = 1U << 2U,
        /** The serial communication started/stopped. */
        serialStateChanged = 1U << 3U,
        /** Change in this flag in the General Setup. */
        mustAddOctaveToMidiInputNoteChanged = 1U << 4U,

        all = static_cast<unsigned int>(-1)
    };

    /** Destructor. */
    virtual ~GeneralDataObserver() = default;

    /**
     * Called when a general data has changed.
     * @param whatChanged what has changed. See the What enum flags.
     */
    virtual void onGeneralDataChanged(unsigned int whatChanged) = 0;

    /** Called when a serial communication event is thrown. The default implementation does nothing */
    virtual void onSerialCommunicationEvent(SerialController::CommunicationState /*state*/, SerialAccess::SerialOperationResult /*reason*/)
    {
        // Nothing to do.
    }
};

}   // namespace arkostracker
