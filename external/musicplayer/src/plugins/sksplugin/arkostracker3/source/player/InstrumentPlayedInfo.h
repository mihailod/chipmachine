#pragma once

#include "../utils/OptionalValue.h"

namespace arkostracker
{

/** Data about what instrument is played, and what part. */
class InstrumentPlayedInfo
{
public:
    /**
     * A snapshot of what instruments were played on what channel. Channels may be absent if playing the Mute sound.
     * @param channelIndexToInstrumentAndPlayedIndex the channel index linked to the instrument ID and the played index (>=0).
     */
    explicit InstrumentPlayedInfo(const std::unordered_map<int, std::pair<OptionalId, int>>& channelIndexToInstrumentAndPlayedIndex);

    const std::unordered_map<int, std::pair<OptionalId, int>>& getInfo() const noexcept;

private:
    std::unordered_map<int, std::pair<OptionalId, int>> channelIndexToInstrumentAndPlayedIndex;
};

}   // namespace arkostracker
