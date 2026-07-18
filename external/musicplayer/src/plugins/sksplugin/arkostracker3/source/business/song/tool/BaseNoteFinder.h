#pragma once

#include "../../../utils/OccurrenceMap.h"

namespace arkostracker 
{

class Cell;
class Id;
class Song;

/** Class to find the base note inside a Subsong. This is useful to find the most used notes, useful for the AKG export. */
class BaseNoteFinder
{
public:
    /**
        Constructor.
        @param song the Song.
        @param subsongId the ID of the Subsong.
        @param noteRange the range, so that baseNote + noteRange = AFTER the last note reachable by the player.
    */
    BaseNoteFinder(const Song& song, const Id& subsongId, int noteRange) noexcept;

    /* Returns the base note. */
    int getBaseNote() const noexcept;

    /** @return the sorted note index and their iteration, from most to least used. */
    std::vector<std::pair<int, unsigned int>> getSortedNoteIndexToIteration() const noexcept; // For each item (note), how many notes were found.

private:
    /** Called by the SubsongBrowser whenever it finds a Cell. */
    void onCellExplored(const Cell& cell) noexcept;

    /** Finds the base note. */
    int findBaseNote(int noteRange) const noexcept;

    /** Calculates the score for this base note index. */
    int calculateScore(int baseNoteIndex, int noteRange) const noexcept;

    int afterLastNoteIndex;                                             // Index of the next-to-last note.
    std::vector<int> noteIndexToIteration;                              // For each item (note), how many notes were found.

    OccurrenceMap<int> occurrenceMap;                                   // Handy to store and manage the notes easily.

    int baseNote;                                                       // The base note, or -1 if not found.
};

}   // namespace arkostracker

