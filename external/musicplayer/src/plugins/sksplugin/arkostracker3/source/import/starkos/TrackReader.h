#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SubsongBuilder;

/** Reads the Tracks/SpecialTracks in the SKS format. */
class TrackReader
{
public:

    /**
     * Parses the SKS to read and creates the Tracks.
     * @param subsongBuilder the builder of the Subsong.
     * @param musicData the data of the song.
     * @param offset the offset of the first Tracks. It must be modified to reach the first byte after it.
     */
    static void readTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& offset) noexcept;

    /**
     * Parses the SKS to read and creates the Special Tracks.
     * @param subsongBuilder the builder of the Subsong.
     * @param musicData the data of the song.
     * @param offset the offset of the first Special Tracks. It must be modified to reach the first byte after it.
     */
    static void readSpecialTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& offset) noexcept;

private:
    static constexpr auto trackMaximumHeight = 128;

    // Values coming from the SKS Tracks.
    //static const int sksMaximumNote = 95;
    static constexpr auto sksVolumeOnly = 96;
    static constexpr auto sksPitchOnly = 97;
    static constexpr auto sksVolumeAndPitchOnly = 98;
    static constexpr auto sksReset = 99;
    static constexpr auto sksDigidrum = 100;

    /**
     * Encodes a Stop Pitch effect in the currently opened Cell.
     * @param subsongBuilder the builder of the Subsong.
     * @param up true if up, false if down.
     */
    static void addStopPitchToCurrentCell(SubsongBuilder& subsongBuilder, bool up) noexcept;
};

}   // namespace arkostracker
