#pragma once

#include <memory>

#include "ChannelData.h"

namespace arkostracker 
{

/**
 * A Pattern composed of tracks (ChannelData), which holds all the registers and their metadata (isNewNote, etc.).
 * There can be 3 Tracks, or many more.
 */
class AkyPattern
{
public:
    /** Constructor. */
    AkyPattern() noexcept;

    /** Adds a Track. It must have the same length as the possible previously added Tracks! */
    void addTrack(std::unique_ptr<ChannelData> track) noexcept;

    /** Returns the length of the Tracks. They are all the same, in a Pattern, in this format! */
    int getTrackLength() const noexcept;

    /** Returns a reference to the Track from its index. */
    ChannelData& getTrack(int trackIndex) const noexcept;

    /** Returns how many Tracks there are in this Pattern. */
    int getTrackCount() const noexcept;

private:
    int length;                                         // The length, in frames.
    std::vector<std::unique_ptr<ChannelData>> tracks;   // The track data.
};

}   // namespace arkostracker

