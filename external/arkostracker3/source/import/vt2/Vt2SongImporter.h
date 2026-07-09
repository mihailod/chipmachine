#pragma once

#include <juce_core/juce_core.h>

#include "../SongImporter.h"
#include "../../business/song/tool/builder/TrailingEffectContext.h"

namespace arkostracker 
{

class SongBuilder;
class SubsongBuilder;

/** Imports a Vortex Tracker 2 text music file. */
class Vt2SongImporter : public SongImporter
{
public:
    /** Constructor. */
    Vt2SongImporter();

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    static const juce::String headerTag;                        // Tag to identify the song format.
    static const int headerTagSize = 8;                         // How long is the header tag.
    static const int maximumOrnamentIndex = 15;                 // Ornaments from 1 to 15, included.
    static const int maximumSampleIndex = 31;                   // Samples from 1 to 31, included.

    /**
     * Parses the music.
     * @param memoryBlock the data of the song.
     * @return the SongBuilder to build the song
    */
    std::unique_ptr<SongBuilder> parse(const juce::MemoryBlock& memoryBlock) noexcept;

    /**
     * Extracts the lines from a section, from its beginning (excluded) to the next empty line or line starting with a "[".
     * @param sectionString the String that starts the section (example: "[Module]").
     * @return the Strings. Empty if not found.
     */
    std::vector<juce::String> extractSectionLines(const juce::String& sectionString) const noexcept;

    /**
     * Extracts a value from the key, inside the given lines, with a "=" as a separator.
     * @param lines the lines to browse.
     * @param key the key.
     * @return the value, or an empty String if none could be found.
     */
    static juce::String extractValue(const std::vector<juce::String>& lines, const juce::String& key) noexcept;

    /** Extracts the indexes and the loopTo index that are contained in a given line, such as "1,2,3,L4,5,1". */
    static std::pair<std::vector<int>, int> extractIndexesAndLoopTo(const juce::String& string) noexcept;

    /**
     * Parses the header.
     * @param songBuilder the Song Builder to use.
     * @return true if everything went fine.
     */
    bool parseHeader(SongBuilder& songBuilder) noexcept;

    /**
     * Parses the ornaments, creating as many Arpeggios.
     * @param songBuilder the Song Builder to use.
     * @return true if everything went fine.
     */
    bool parseOrnaments(SongBuilder& songBuilder) const noexcept;

    /**
     * Parses the samples, creating as many Instruments.
     * @param songBuilder the Song Builder to use.
     * @return true if everything went fine.
     */
    bool parseSamples(SongBuilder& songBuilder) const noexcept;

    /**
     * Parses the Pattens, creating Tracks.
     * @param songBuilder the Song Builder to use.
     * @return true if everything went fine.
     */
    bool parsePatterns(SongBuilder& songBuilder) noexcept;

    /**
     * Parses the given Pattern Line, and stores the generated Cells in Tracks that may or may not exist.
     * @param songBuilder the Song Builder to use.
     * @param channelContexts the contexts, one for each channel.
     * @param lineIndex the line index.
     * @param lineToParse the line to parse.
     * @param patternIndex the Pattern index.
     * @return true if everything went fine.
     */
    static bool parsePatternLine(SongBuilder& songBuilder, std::vector<TrailingEffectContext>& channelContexts,
                          int lineIndex, const juce::String& lineToParse, int patternIndex) noexcept;

    /**
     * Parses the Track Cell, and stores the generated Cells in Tracks that may or may not exist.
     * @param songBuilder the Song Builder to use.
     * @param patternIndex the index of the Pattern.
     * @param channelContext the context for this track, to know if the latest Instrument for example.
     * @param lineIndex the line index.
     * @param cellToParse the cell to parse.
     * @param trackIndex the Track index.
     * @return true if everything went fine.
     */
    static bool parseTrackCell(SongBuilder& songBuilder, int patternIndex, TrailingEffectContext& channelContext, int lineIndex,
                               const juce::String& cellToParse, int trackIndex) noexcept;

    /**
     * Parses the given effect column and fills the given CellEffects, if needed.
     * @param subsongBuilder the Subsong Builder to use.
     * @param patternIndex the index of the Pattern.
     * @param effectColumn the effect column.
     * @param cell the Cell to fill. May already contains volume and arpeggio effect.
     * @param cellIndex the index of the Cell.
     */
    static void parseEffectColumn(SubsongBuilder& subsongBuilder, int patternIndex, const juce::String& effectColumn, Cell& cell, int cellIndex) noexcept;

    /**
     * Manages a pitch or glide effect.
     * @param valueString the raw String value from the column. 3 digits.
     * @param effect the effect to encode.
     * @param multiplier a multiplier for the encoded value.
     * @param cell the Cell to fill. May already contains volume and arpeggio effect.
     */
    static void managePitchOrGlideEffect(const juce::String& valueString, Effect effect, double multiplier, Cell& cell) noexcept;

    /**
     * Declares a new Speed.
     * @param subsongBuilder the Subsong Builder to use.
     * @param patternIndex the index of the Pattern.
     * @param speed the new Speed. Must be valid.
     * @param cellIndex the index of the Cell.
     */
    static void declareSpeed(SubsongBuilder& subsongBuilder, int patternIndex, int speed, int cellIndex) noexcept;

    /** Fills the SpeedTrack index and height of all the Patterns. Must be called after the parsePatternsAndPositions has been done. */
    void fillPatternMetadata(SubsongBuilder& subsongBuilder) noexcept;

    std::vector<juce::String> readLines;                        // The read lines from the file.
    std::set<int> usedPatternIndexes;                           // The Pattern indexes that are used.

    int patternCount;                                           // How many Patterns there are in the Song.

    std::map<int, int> patternIndexToHeight;                    // Links a Pattern index (NOT a position!) to its height.
    std::vector<int> positionToPatternIndex;                    // Links a position to its Pattern index.
};


}   // namespace arkostracker

