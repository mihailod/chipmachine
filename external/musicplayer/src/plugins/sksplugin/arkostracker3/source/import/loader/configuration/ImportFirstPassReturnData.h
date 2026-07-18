#pragma once

#include <set>

namespace arkostracker 
{

/**
 * A simple class to hold data that may be returned from the first pass of loading a Song, in order to show it on an Import Panel.
 * For example a Midi loader will extract some data, to be shown in the import panel.
 *
 * No need of a class hierarchy, let's keep it simple...
 */
class ImportFirstPassReturnData
{
public:
    /** Default constructor. */
    ImportFirstPassReturnData() noexcept :
            trackCount(0),
            midiSongPpq(0),
            midiTrackIndexesWithNotes()
    {
    }

    /**
     * Constructor. Use the builder instead for more convenience.
     * @param pTrackCount how many tracks there are.
     * @param pSongPpq the song ppq.
     * @param pTrackIndexesWithNotes the indexes of the tracks that have notes.
     */
    ImportFirstPassReturnData(int pTrackCount, int pSongPpq, std::set<int> pTrackIndexesWithNotes) noexcept :
            trackCount(pTrackCount),
            midiSongPpq(pSongPpq),
            midiTrackIndexesWithNotes(std::move(pTrackIndexesWithNotes))
    {
    }

    /**
     * Constructor for MIDI.
     * @param pTrackCount how many tracks there are.
     * @param pSongPpq the song ppq.
     * @param pTrackIndexesWithNotes the indexes of the tracks that have notes.
     */
    static ImportFirstPassReturnData buildForMidi(int pTrackCount, int pSongPpq, std::set<int> pTrackIndexesWithNotes) noexcept
    {
        return { pTrackCount, pSongPpq, std::move(pTrackIndexesWithNotes) };
    }

    /**
     * Constructor for MOD.
     * @param pTrackCount how many tracks there are.
     */
    static ImportFirstPassReturnData buildForMod(int pTrackCount) noexcept
    {
        return { pTrackCount, 0, { } };
    }

    /** @return how many tracks there are, originally. */
    int getTrackCount() const noexcept
    {
        return trackCount;
    }

    /** @return the song ppq. */
    int getMidiSongPpq() const noexcept
    {
        return midiSongPpq;
    }

    /** @return the indexes of the tracks that have notes. */
    const std::set<int>& getMidiTrackIndexesWithNotes() const noexcept
    {
        return midiTrackIndexesWithNotes;
    }

private:
    int trackCount;
    int midiSongPpq;
    std::set<int> midiTrackIndexesWithNotes;
};



}   // namespace arkostracker

