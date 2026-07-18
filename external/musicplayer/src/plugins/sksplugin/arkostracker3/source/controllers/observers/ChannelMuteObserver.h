#pragma once

#include <unordered_set>

namespace arkostracker 
{

/** Observer for objects wanting to be notified when a channel is muted or not. */
class ChannelMuteObserver
{
public:
    /** Destructor. */
    virtual ~ChannelMuteObserver() = default;

    /**
     * Called when the state of channel mute state has changed.
     * @param mutedChannelIndexes the indexes of the channels which are mute. All the others are un-muted.
     */
    virtual void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) = 0;
};

}   // namespace arkostracker

