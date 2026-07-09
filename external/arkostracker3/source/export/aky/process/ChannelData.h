#pragma once

#include <memory>
#include <vector>

#include "../../../player/channel/ChannelPlayerResults.h"

namespace arkostracker 
{

/** The list of ChannelRegisters, plus their metadata. */
class ChannelData
{
public:
    /** Constructor. */
    ChannelData() noexcept;

    /**
     * Adds a ChannelPlayerResults to the data.
     * @param channelPlayerResults the data to add.
     */
    void addChannelPlayerResults(std::unique_ptr<ChannelPlayerResults> channelPlayerResults) noexcept;

    /** @return the size of the list. */
    int getSize() const noexcept;

    /**
     * @return the data according to the given index.
     * @param index the index. Must be valid.
     */
    const ChannelPlayerResults& getData(int index) const noexcept;

private:
    std::vector<std::unique_ptr<ChannelPlayerResults>> channelPlayerResultsList;                                    // The data.
};

}   // namespace arkostracker

