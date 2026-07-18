#pragma once

#include <unordered_set>

namespace arkostracker
{

/** Logic when muting channels. */
class ChannelsMute
{
public:
    /**
     * Rules on toggling between a solo and mute all. This is typically the right-click on a Channel.
     * @param mutedChannelIndexes the indexes of the muted channels.
     * @param selectedChannelIndex the channel on which the action is performed.
     * @param channelCount how many channels there are (>0, typically 3).
     * @return a set of the muted channels, if any.
     */
    std::unordered_set<int> toggleSoloOrUnmuteAll(const std::unordered_set<int>& mutedChannelIndexes, int selectedChannelIndex, int channelCount) const noexcept;

private:
    /**
     * @returns a set of muted channels, except the selected one.
     * @param selectedChannelIndex the channel on which the action is performed.
     * @param channelCount how many channels there are (>0, typically 3).
     */
    std::unordered_set<int> soloChannel(int selectedChannelIndex, int channelCount) const noexcept;
};

}   // namespace arkostracker
