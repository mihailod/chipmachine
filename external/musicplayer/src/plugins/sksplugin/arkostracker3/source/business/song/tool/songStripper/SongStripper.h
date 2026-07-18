#pragma once

#include <memory>

#include "SubsongsAndPsgs.h"

namespace arkostracker 
{

class Song;
class Id;

/**
 * Class to copy a Song into one another, stripped of Subsongs and/or PSGs.
 * This is useful to remove parts of a Song before making an export, such as AKY, which only supports one Subsong.
 * The original Song is copied and thus not modified.
 *
 * Note that the new Song is NOT optimized in any way. Tracks are deleted if their related Subsongs is deleted,
 * but there might be unused Tracks if only PSGs are deleted.
 */
class SongStripper          // TODO TU this.
{
public:
    /** Prevents instantiation. */
    SongStripper() = delete;

    /**
     * Splits the Song into another Song, removed of Subsongs/PSGs.
     * @param song the original Song.
     * @param subsongsAndPsgsToKeep the Subsongs and their PSGs to keep.
     * @return the new Song. The Subsong IDs are the same.
     */
    static std::unique_ptr<Song> split(const Song& song, const SubsongsAndPsgs& subsongsAndPsgsToKeep) noexcept;

private:
    /**
     * Removes the PSGs which are now in the given indexes. They may not be present already.
     * This will not remove the Tracks.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param subsongsAndPsgsToKeep the Subsongs and their PSGs to keep.
     */
    static void managePsg(Song& song, const Id& subsongId, const SubsongsAndPsgs& subsongsAndPsgsToKeep) noexcept;
};


}   // namespace arkostracker

