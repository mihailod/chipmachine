#include "PsgData.h"

namespace arkostracker
{

PsgData::PsgData(unsigned int pPsgFrequency, float pReplayFrequency) noexcept :
    psgFrequency(pPsgFrequency),
    replayFrequency(pReplayFrequency)
{
}

unsigned int PsgData::getPsgFrequency() const noexcept
{
    return psgFrequency;
}

float PsgData::getReplayFrequency() const noexcept
{
    return replayFrequency;
}

}   // namespace arkostracker
