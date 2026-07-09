#pragma once

#include <set>
#include <utility>
#include <vector>

namespace arkostracker 
{

/** Holds channels to mix (4 into 3, 2 into 4, etc.), and at the end, which one to keep. */
class ChannelMixerKeeper
{
public:
    /** Constructor. */
    ChannelMixerKeeper();

    /**
       Stores an action of mixing a track to another.
       @param sourceTrackIndex the index of the source track.
       @param destinationTrackIndex the index of the destination track.
    */
    void addTrackMix(int sourceTrackIndex, int destinationTrackIndex) noexcept;

    /**
       Stores an action of keeping a track.
       @param trackIndexToKeep the index of the track to keep.
    */
    void addTrackToKeep(int trackIndexToKeep) noexcept;

    /**
       Stores an action of keeping a track.
       @param trackIndexesToKeep the index of the track to keep.
    */
    void addTracksToKeep(const std::set<int>& trackIndexesToKeep) noexcept;

    /**
       Returns the list of tracks to mix.
       @return the pair of indexes (source, destination).
    */
    const std::vector<std::pair<int, int>>& getTrackMixes() const noexcept;

    /** Returns the indexes of the tracks to keep. */
    const std::set<int>& getTrackIndexesToKeep() const noexcept;

    /** Returns how many tracks are count. */
    int getKeptTrackCount() const noexcept;

private:
    std::vector<std::pair<int, int>> sourceToDestinationTrackIndexes;       // Indexes of the tracks to mix into destination indexes.
    std::set<int> trackIndexesToKeep;                                       // Indexes of the tracks to keep in the original song.
};

}   // namespace arkostracker
