#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SongBuilder;

/** Reads the Instruments in the SKS format. */
class InstrumentReader
{
public:

    /**
     * Parses the SKS to read and creates the Instruments.
     * @param songBuilder the builder of the Song.
     * @param memoryData the data of the song.
     * @param offset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     */
    static void readInstruments(SongBuilder& songBuilder, const juce::MemoryBlock& memoryData, juce::int64& offset) noexcept;

private:
    static constexpr auto sizeInstrumentName = 8;

    /**
     * Parses the SKS to read and creates the Instrument Cells.
     * @param songBuilder the builder of the Song.
     * @param memoryBlock the data of the song.
     * @param offset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     * @param instrumentIndex the index of the Instrument.
     */
    static void readInstrumentCells(SongBuilder& songBuilder, const juce::MemoryBlock& memoryBlock, juce::int64& offset, int instrumentIndex) noexcept;

    /**
     * Reads a software Cell. It is already opened.
     * @param songBuilder the builder of the Song.
     * @param memoryBlock the data of the song.
     * @param offset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     * @param firstByte the first byte of the cell.
     * @param instrumentIndex the index of the Instrument.
     */
    static void readSoftwareCell(SongBuilder& songBuilder, const juce::MemoryBlock& memoryBlock, juce::int64& offset, unsigned int firstByte, int instrumentIndex) noexcept;

    /**
     * Reads a hardware Cell. It is already opened.
     * @param songBuilder the builder of the Song.
     * @param musicData the data of the song.
     * @param offset the offset of the first Instrument Cells. It must be modified to reach the first byte after it.
     * @param firstByte the first byte of the cell.
     * @param instrumentIndex the index of the Instrument.
     */
    static void readHardwareCell(SongBuilder& songBuilder, const juce::MemoryBlock& musicData, juce::int64& offset, unsigned int firstByte, int instrumentIndex) noexcept;
};

}   // namespace arkostracker
