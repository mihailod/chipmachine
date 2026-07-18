#include "InstrumentPlayedInfo.h"

namespace arkostracker
{
InstrumentPlayedInfo::InstrumentPlayedInfo(const std::unordered_map<int, std::pair<OptionalId, int>>& pChannelIndexToInstrumentAndPlayedIndex) :
        channelIndexToInstrumentAndPlayedIndex(pChannelIndexToInstrumentAndPlayedIndex)
{
}

const std::unordered_map<int, std::pair<OptionalId, int>>& InstrumentPlayedInfo::getInfo() const noexcept
{
    return channelIndexToInstrumentAndPlayedIndex;
}

}   // namespace arkostracker
