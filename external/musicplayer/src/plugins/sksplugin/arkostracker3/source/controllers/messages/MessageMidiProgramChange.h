#pragma once

#include "../../utils/message/MessageWithOneParameter.h"

namespace arkostracker 
{

/** Simple implementation to transmit a Program Change from the audio thread. */
class MessageMidiProgramChange final : public MessageWithOneParameter<int>
{
public:
    /**
    * Constructor.
    * @param programChangeNumber the number of the Program Change.
    */
    explicit MessageMidiProgramChange(const int programChangeNumber) noexcept :
        MessageWithOneParameter(programChangeNumber)
    {
    }
};

}   // namespace arkostracker
