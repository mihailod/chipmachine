#include "ChannelsMute.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

std::unordered_set<int> ChannelsMute::toggleSoloOrUnmuteAll(const std::unordered_set<int>& mutedChannelIndexes, const int selectedChannelIndex,  // NOLINT(readability-convert-member-functions-to-static)
                                                            const int channelCount) const noexcept
{
    jassert(selectedChannelIndex < channelCount);

    // Simple cases first:
    // If nothing is muted, solos the selected channel.
    // If the selected channel is muted, solo it.
    if (mutedChannelIndexes.empty() || (mutedChannelIndexes.find(selectedChannelIndex) != mutedChannelIndexes.cend())) {
        return soloChannel(selectedChannelIndex, channelCount);
    }

    // If ALL the other channels are muted, unmute all. Else, solo selected channel.
    bool canUnmuteAllBePerformed = true;
    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        if (channelIndex == selectedChannelIndex) {
            // We have already treated the selected channel, skips it.
            continue;
        }

        if (mutedChannelIndexes.find(channelIndex) == mutedChannelIndexes.cend()) {
            // The channel is not muted.
            canUnmuteAllBePerformed = false;
            break;
        }
    }

    if (canUnmuteAllBePerformed) {
        // Unmute all.
        return { };
    }

    return soloChannel(selectedChannelIndex, channelCount);
}

std::unordered_set<int> ChannelsMute::soloChannel(const int selectedChannelIndex, const int channelCount) const noexcept // NOLINT(readability-convert-member-functions-to-static)
{
    std::unordered_set<int> result;

    // All are muted (present in the map), except the select channel.
    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        if (channelIndex != selectedChannelIndex) {
            result.insert(channelIndex);
        }
    }

    return result;
}


}   // namespace arkostracker

