#include "AkgPattern.h"

#include "../../../song/subsong/Pattern.h"
#include "../../../song/subsong/Position.h"

namespace arkostracker 
{

AkgPattern::AkgPattern(const Position& position, const Pattern& pattern) noexcept :
        height(position.getHeight()),
        trackItems(),
        speedTrackIndex(pattern.getCurrentSpecialTrackIndex(true)),
        eventTrackIndex(pattern.getCurrentSpecialTrackIndex(false))
{
    auto channelIndex = 0;
    for (auto trackIndex : pattern.getCurrentTrackIndexes()) {
        const auto transposition = position.getTransposition(channelIndex);
        trackItems.emplace_back(trackIndex, transposition);

        ++channelIndex;
    }
}

bool AkgPattern::operator==(const AkgPattern& pattern) const
{
    return (height == pattern.height)
           && (speedTrackIndex == pattern.speedTrackIndex)
           && (eventTrackIndex == pattern.eventTrackIndex)
           && (trackItems == pattern.trackItems);
}

int AkgPattern::getHeight() const noexcept
{
    return height;
}

int AkgPattern::getSpeedTrackIndex() const noexcept
{
    return speedTrackIndex;
}

int AkgPattern::getEventTrackIndex() const noexcept
{
    return eventTrackIndex;
}

int AkgPattern::getChannelCount() const noexcept
{
    return static_cast<int>(trackItems.size());
}

int AkgPattern::getTransposition(const int channelIndex) const noexcept
{
    return trackItems.at(static_cast<size_t>(channelIndex)).getTransposition();
}

}   // namespace arkostracker
