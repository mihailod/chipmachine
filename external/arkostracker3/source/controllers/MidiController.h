#pragma once

#include "../utils/Observers.h"

namespace arkostracker 
{

/** Abstract Controller of the Midi. */
class MidiController
{
public:
    /** Listener to be aware of midi events. */
    class MidiListener
    {
    public:
        /** Destructor. */
        virtual ~MidiListener() = default;

        /**
         * Called when a midi note is received. This is called on the UI thread.
         * @param noteNumber the note.
         */
        virtual void onMidiNoteReceived(int noteNumber) = 0;

        /**
         * Called when a Program Change is received. This is called on the UI thread.
         * @param programChange the program change.
         */
        virtual void onMidiProgramChangeReceived(int programChange) = 0;
    };

    /** Destructor. */
    virtual ~MidiController() = default;

    /** Scans the Midi devices and register to those which have Midi In enabled. Must be done only when the AudioDeviceManager has been also setup. */
    virtual void scanMidiDevicesAndRegister() noexcept = 0;

    /** @return the observers to the Midi events. */
    virtual Observers<MidiListener>& getMidiObservers() noexcept = 0;
};

}   // namespace arkostracker
