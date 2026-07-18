#pragma once

#include <memory>
#include <unordered_set>

#include "../SongImporter.h"
#include "../../business/song/tool/builder/SongBuilder.h"

namespace arkostracker 
{

/** Imports a Soundtrakker 128 song. */
class Stk128Importer final : public SongImporter
{
public:
    /** Constructor. */
    Stk128Importer() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    static const juce::String extensionStk;                 // The Soundtrakker extension, with the dot.

    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param musicData the music data.
     */
    void parse(const juce::MemoryBlock& musicData) noexcept;

    /**
     * Reads all the instruments and puts them in the song. Also fills the "S instruments table".
     * @param musicData the data of the song.
     * @return true if no critical error was found.
     */
    bool readAndCreateAllInstruments(const juce::MemoryBlock& musicData) noexcept;

    /**
     * Reads one instrument and puts it in the song. Also fills the "S instruments table" if the Instrument fits.
     * @param instrumentIndex the instrument index to read (from 0 to 15 included).
     * @param musicData the data of the song.
     * @return true if no critical error was found.
     */
    bool readAndCreateInstrument(int instrumentIndex, const juce::MemoryBlock& musicData) noexcept;

    /**
     * Reads all the arpeggios and puts them in the song.
     * @param musicData the data of the song.
     * @return true if no critical error was found.
     */
    bool readAndCreateAllArpeggios(const juce::MemoryBlock& musicData) const noexcept;

    /**
        Reads one arpeggio and put it in the song.
        @param arpeggioIndex the arpeggio index to read (from 0 to 15 included).
        @param musicData the data of the song.
        @return true if no critical error was found.
    */
    bool readAndCreateArpeggio(int arpeggioIndex, const juce::MemoryBlock& musicData) const noexcept;

    /**
     * Adds one Subsong into the Song, and populates its tracks.
     * @param musicData the data of the song.
     * @return true if no critical error was found.
     */
    bool readAndCreateSubsong(const juce::MemoryBlock& musicData) const noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
    std::unordered_set<int> instrumentsWithSFlagIndexes;            // Indexes (>0) of Soundtrakker Instruments with a "S" flag (full volume).
};


}   // namespace arkostracker

