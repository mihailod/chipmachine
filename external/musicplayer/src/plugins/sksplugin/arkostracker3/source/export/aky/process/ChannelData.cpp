#include "ChannelData.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

ChannelData::ChannelData() noexcept :
    channelPlayerResultsList()
{
}

void ChannelData::addChannelPlayerResults(std::unique_ptr<ChannelPlayerResults> channelPlayerResults) noexcept
{
    channelPlayerResultsList.push_back(std::move(channelPlayerResults));
}

int ChannelData::getSize() const noexcept
{
    return static_cast<int>(channelPlayerResultsList.size());
}

const ChannelPlayerResults& ChannelData::getData(const int index) const noexcept
{
    jassert(index < static_cast<int>(channelPlayerResultsList.size()));

    return *channelPlayerResultsList.at(static_cast<size_t>(index));
}

}   // namespace arkostracker

