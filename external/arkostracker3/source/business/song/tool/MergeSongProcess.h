#pragma once

#include "../../../song/Song.h"

namespace arkostracker
{

class MergeSongProcess
{
public:
    /** Prevents instantiation. */
    MergeSongProcess() = delete;

    /**
     * @return a new song, merge from the given one, or nullptr if too many instruments or expressions.
     * @param originalSong the original Song.
     * @param songToMerge the song to merge.
     */
    static std::unique_ptr<Song> merge(const Song& originalSong, const Song& songToMerge) noexcept;

private:
    /** Adds the expressions from the song to merge to the given one. */
    static void addExpressions(bool isArpeggio, Song& currentSong, const Song& songToMerge) noexcept;

    /** Shifts the expressions in the given cell, if any. */
    static bool shiftExpressions(const CellEffects& cellEffects, Effect effect, int expressionCount, Cell& cell) noexcept;
};

}   // namespace arkostracker
