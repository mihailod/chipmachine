#pragma once

#include <juce_core/juce_core.h>

#include "../SongImporter.h"

namespace arkostracker 
{

class SongBuilder;

/** Importer of a "chip'n'sfx" song. */
class ChpSongImporter : public SongImporter
{
public:
    /** Constructor. */
    ChpSongImporter();

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    static const int glideValue;                                    // The glide value used when the Instrument 0 is used.
    static const juce::String headerTag;                            // Tag to identify the song format.
    static const size_t lineIndexLinker;                            // Line where the linker starts.

    /**
     * Parses the whole song.
     * @param memoryBlock the data of the song.
     * @return the SongBuilder to build the song.
     */
    std::unique_ptr<SongBuilder> parse(const juce::MemoryBlock& memoryBlock) noexcept;

    /**
     * Finds the separators in the read lines, allowing to find the Tracks and Instruments parts.
     * @return true if everything went fine.
     */
    bool findPartsFromSeparator() noexcept;

    /**
     * Finds a separator from the given start line.
     * @param startLineIndex the line index to make the search from.
     * @return the line index, or 0 if there is none.
     */
    size_t findSeparator(size_t startLineIndex) const noexcept;

    /**
     * Parses the header.
     * @param songBuilder the Song Builder.
     * @return false if a critical error happened.
     */
    bool parseHeader(SongBuilder& songBuilder) noexcept;

    /**
    * Parses the Tracks.
    * @param songBuilder the Song Builder.
    * @return false if a critical error happened.
    */
    bool parseTracks(SongBuilder& songBuilder) noexcept;

    /**
     * Parses the given Track Cell, adds it to the Subsong.
     * @param songBuilder the Song Builder.
     * @param trackCell the Track cell text, as read from the text file ("^^^:.." or "B-1:05").
     * @param trackIndex the index of the Track.
     * @param cellIndex the index of the Cell.
     * @return false if a critical error happened.
     */
    bool parseTrackCell(SongBuilder& songBuilder, const juce::String& trackCell, int trackIndex, int cellIndex) const noexcept;

    /**
     * Parses the Linker. The Tracks must have been parsed before, to know their heights.
     * @param songBuilder the Song Builder.
     * @return false if a critical error happened.
     */
    bool parseLinker(SongBuilder& songBuilder) noexcept;

    /**
     * Extracts the track index and transposition from the given Linker Cell ("#0A+00" for example).
     * @return the track index (-1 if an error occurred), and the transposition.
     */
    static std::pair<int, int> extractTrackIndexAndTranspositionFromLinkerCell(const juce::String& linkerCell) noexcept;

    /**
     * Parses the Instruments. The linker and Tracks must have been parsed.
     * @param songBuilder the Song Builder.
     * @return false if a critical error happened.
     */
    bool parseInstruments(SongBuilder& songBuilder) const noexcept;

    /**
     * Parses the given Instrument, using all the splits of the line.
     * @param songBuilder the Song Builder.
     * @param readLine the line to split.
     * @return the line index, or 0 if there is none.
     */
    static bool parseInstrument(SongBuilder& songBuilder, const juce::String& readLine) noexcept;

    /**
     * Returns the arpeggio from the given arpeggio, as the input value may have a specific meaning.
     * @param arpeggio the arpeggio from 0 to 15.
     * @return the arpeggio.
     */
    static int getArpeggioFromDigit(int arpeggio) noexcept;

    std::vector<juce::String> readLines;                        // The read lines from the file.
    int globalTransposition;                                    // How much to transpose each note.
    size_t lineIndexOfLinker;                                   // The line index where the Linker starts.
    size_t lineIndexOfTracks;                                   // The line index where the Tracks are.
    size_t lineIndexOfInstruments;                              // The line index where the Instruments are. -1 if not known yet. Filled when the Tracks are read.

    std::map<int, int> trackIndexToLastCellIndexUsed;           // Links a Track index to the index of its last cell used. Used to get the height of the Patterns.
};

}   // namespace arkostracker
