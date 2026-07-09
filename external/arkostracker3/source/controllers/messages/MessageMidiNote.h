#pragma once

#include "../../utils/message/MessageWithOneParameter.h"

namespace arkostracker 
{

/** Simple implementation to transmit a new Midi note from the audio thread. */
class MessageMidiNote final : public MessageWithOneParameter<int>
{
public:
    /**
    * Constructor.
    * @param midiNote the midi note.
    */
    explicit MessageMidiNote(const int midiNote) noexcept :
        MessageWithOneParameter(midiNote)
    {
    }
};

}   // namespace arkostracker
