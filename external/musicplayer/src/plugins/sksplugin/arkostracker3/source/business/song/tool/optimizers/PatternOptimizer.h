#pragma once

#include "../../../../song/Song.h"
#include "../../../../song/subsong/Pattern.h"
#include "../../../../song/subsong/Position.h"
#include "../../../../song/tracks/SpecialTrack.h"
#include "../../../../song/tracks/Track.h"
#include "../../../../utils/Id.h"

namespace arkostracker 
{

class Song;
class Subsong;

/**
 * Generates a new Song from the given one, with optimized Patterns (only). This does NOT optimize the Instruments/Expressions, which are
 * still referred in the same way (use the Expression/Instruments Optimizers for that).
 *
 * Such generated songs must NOT be directly used by AT3. It must be Unoptimized before use so that each Pattern has only its own unique Tracks, not multi-referenced ones.
 *
 * The generated songs are perfect for exports, which themselves may perform more changes (legato change, arpeggio un-inlining, then re-optimization).
 */
class PatternOptimizer
{
public:
    /**
     * Constructor.
     * @param originalSong the original Song to optimize.
     * @param subsongIdsToKeep the IDs of the Subsongs to keep.
     */
    PatternOptimizer(const Song& originalSong, std::vector<Id> subsongIdsToKeep) noexcept;

    /**
     * Optimizes the Song. Must be called only once.
     * @param keepOnlyTillSubsongLoopEnd true to keep the Patterns only up the loop end, false to keep up the full end.
     * @param normalizeCellsAndOptimizeTracks true to normalize the Cells, and optimize the Tracks (remove useless effects). True for exporters, false for within the software.
     * @return the new Song.
     */
    std::unique_ptr<Song> optimize(bool keepOnlyTillSubsongLoopEnd, bool normalizeCellsAndOptimizeTracks) noexcept;

private:
    void createSortedAndCorrectedAndReducedTracks(bool normalizeCells) noexcept;

    /**
     * @return a normalized Track (reduced to the given height).
     * @param originalTrack the original track.
     * @param normalizeCellsAndOptimizeTracks true to normalize the Cells, and optimize the Tracks (remove useless effects).
     * @param positionHeight the position height to reduce the given Track to.
     */
    static Track normalizeTrack(const Track& originalTrack, bool normalizeCellsAndOptimizeTracks, int positionHeight) noexcept;

    /**
     * @return a normalized SpecialTrack (reduced to the given height).
     * @param originalTrack the original track.
     * @param normalizeCells this is actually not used, but required for generics.
     * @param positionHeight the position height to reduce the given Track to.
     */
    static SpecialTrack normalizeTrack(const SpecialTrack& originalTrack, bool normalizeCells, int positionHeight) noexcept;

    /**
     * Sorts the given Tracks, from tallest to smallest.
     * @tparam TYPE the type of the items.
     * @param tracks the Tracks, unsorted.
     */
    template<typename TYPE>
    static void sortTracks(std::vector<AbstractTrack<TYPE>>& tracks) noexcept
    {
        // Sorts the tracks from tallest to smallest.
        std::sort(tracks.begin(), tracks.end(), [&](const AbstractTrack<TYPE>& left, const AbstractTrack<TYPE>& right) {
            return left.findMinimumHeight() > right.findMinimumHeight();
        });
    }

    template<typename TYPE>
    static OptionalInt findOptimizedTrack(const std::vector<TYPE>& sortedTracks, std::unordered_map<int, int>& sortedTrackIndexToNew,
                                          const int originalTrackIndex, const std::vector<TYPE>& originalTracks,
                                          bool normalizeCells, int positionHeight,
                                          std::vector<TYPE>& finalTracks) noexcept
    {
        const auto& fullOriginalTrack = originalTracks.at(static_cast<size_t>(originalTrackIndex));
        const auto normalizedOriginalTrack = normalizeTrack(fullOriginalTrack, normalizeCells, positionHeight);

        // Finds a new Track the original one is included to.
        auto sortedTrackIndex = 0;
        auto found = false;
        for (const auto& sortedTrack : sortedTracks) {
            if (normalizedOriginalTrack.isContainedIn(positionHeight, sortedTrack)) {
                found = true;
                break;
            }
            ++sortedTrackIndex;
        }
        if (!found) {
            return { };
        }

        // Has the sorted track index already been used? If yes, re-use its final index.
        auto finalTrackIndex = 0;
        if (const auto it = sortedTrackIndexToNew.find(sortedTrackIndex); it == sortedTrackIndexToNew.cend()) {
            // Not used yet. The final index if the size of the map.
            finalTrackIndex = static_cast<int>(sortedTrackIndexToNew.size());

            sortedTrackIndexToNew.emplace(sortedTrackIndex, finalTrackIndex);
            // Duplicates the Tracks in the right order (linear).
            finalTracks.push_back(sortedTracks.at(static_cast<size_t>(sortedTrackIndex)));
        } else {
            // Already used.
            finalTrackIndex = sortedTrackIndexToNew[sortedTrackIndex];
        }

        return finalTrackIndex;
    }

    /**
     * Checks whether there are speed change, and if they are not equal to the initial speed, or
     * if the first used speed track has a different value, in which case the initial speed can be
     * changed, and all speed removed.
     * @param subsong the Subsong to optimize. Will be modified.
     * @param keepOnlyTillSubsongLoopEnd true to stop at the loop end, false to stop at the end of the subsong.
     */
    static void optimizeSpeedTracks(Subsong& subsong, bool keepOnlyTillSubsongLoopEnd) noexcept;

    Song originalSong;
    std::vector<Id> originalSubsongIds;

    std::vector<Position> originalPositions;
    std::vector<Pattern> originalPatterns;
    std::vector<Track> originalTracks;
    std::vector<SpecialTrack> originalSpeedTracks;
    std::vector<SpecialTrack> originalEventTracks;

    std::vector<Track> sortedTracks;                    // Filled, then sorted from highest to smallest.
    std::vector<SpecialTrack> sortedSpeedTracks;        // Filled, then sorted from highest to smallest.
    std::vector<SpecialTrack> sortedEventTracks;        // Filled, then sorted from highest to smallest.

};

}   // namespace arkostracker
