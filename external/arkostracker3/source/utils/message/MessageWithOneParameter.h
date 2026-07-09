#pragma once

#include <juce_events/juce_events.h>

namespace arkostracker 
{

/** Generic JUCE message, with one parameter. Useful to transmit data from a worker thread to a listener on the UI thread for example. */
template<typename PARAMETER>
class MessageWithOneParameter : public juce::Message
{
public:
    /**
     * Constructor.
     * @param pData the data.
     */
    explicit MessageWithOneParameter(PARAMETER pData) noexcept:
            data(pData)
    {
    }

    /** Returns the stored data. */
    PARAMETER getData() const noexcept
    {
        return data;
    }

private:
    PARAMETER data;                  // The stored data.
};

}   // namespace arkostracker
