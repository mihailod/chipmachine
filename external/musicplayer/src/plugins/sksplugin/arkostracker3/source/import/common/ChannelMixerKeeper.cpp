#include "ChannelMixerKeeper.h"

namespace arkostracker 
{

ChannelMixerKeeper::ChannelMixerKeeper() :
        sourceToDestinationTrackIndexes(),
        trackIndexesToKeep()
{
}

void ChannelMixerKeeper::addTrackMix(int sourceTrackIndex, int destinationTrackIndex) noexcept
{
    std::pair pair(sourceTrackIndex, destinationTrackIndex);
    sourceToDestinationTrackIndexes.push_back(pair);
}

void ChannelMixerKeeper::addTrackToKeep(const int trackIndexToKeep) noexcept
{
    trackIndexesToKeep.insert(trackIndexToKeep);
}

void ChannelMixerKeeper::addTracksToKeep(const std::set<int>& trackIndexesToAdd) noexcept
{
    trackIndexesToKeep.insert(trackIndexesToAdd.cbegin(), trackIndexesToAdd.cend());
}

const std::vector<std::pair<int, int>>& ChannelMixerKeeper::getTrackMixes() const noexcept
{
    return sourceToDestinationTrackIndexes;
}

const std::set<int>& ChannelMixerKeeper::getTrackIndexesToKeep() const noexcept
{
    return trackIndexesToKeep;
}

int ChannelMixerKeeper::getKeptTrackCount() const noexcept
{
    return static_cast<int>(trackIndexesToKeep.size());
}

}   // namespace arkostracker
