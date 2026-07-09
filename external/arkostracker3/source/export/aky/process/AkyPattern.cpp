#include "AkyPattern.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

AkyPattern::AkyPattern() noexcept :
    length(0),
    tracks()
{
}

void AkyPattern::addTrack(std::unique_ptr<ChannelData> track) noexcept
{
    const auto currentLength = track->getSize();
    tracks.push_back(std::move(track));

    // The Tracks should be all the same size! Illogical if they are not.
    jassert(tracks.at(0U)->getSize() == currentLength);  // Takes care of the case of a first addition.

    length = currentLength;
}

int AkyPattern::getTrackLength() const noexcept
{
    return length;
}

ChannelData& AkyPattern::getTrack(const int trackIndex) const noexcept
{
    jassert(trackIndex < static_cast<int>(tracks.size()));
    return *tracks.at(static_cast<size_t>(trackIndex));
}

int AkyPattern::getTrackCount() const noexcept
{
    return static_cast<int>(tracks.size());
}

}   // namespace arkostracker

