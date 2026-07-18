#pragma once

#include <juce_core/juce_core.h>

#include "../SongImporter.h"
#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../business/song/tool/builder/TrailingEffectContext.h"

namespace arkostracker 
{

class ModTrackCell;

/** Imports a MODule. */
class ModSongImporter final : public SongImporter
{
public:
    /** Constructor. */
    ModSongImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;
    ConfigurationType requireConfiguration() const noexcept override;
    ImportFirstPassReturnData getReturnData() const noexcept override;

private:
    static const juce::String modExtension;                  // The extension, without the dot.

    static constexpr auto defaultSampleFrequency = 8373;          // The frequency of the samples, in Hz.

    static constexpr auto offsetSongTitle = 0;           // Offset of the title.
    static constexpr auto sizeSongTitle = 20;            // Size of the title, in bytes.
    static constexpr auto offsetSamplesMetadata = offsetSongTitle + sizeSongTitle;
    static constexpr auto sizeSampleTitle = 22;          // Size of a title of a sample.
    static constexpr auto sizeSampleMetadata = sizeSampleTitle + 8;       // Size of the metadata block for one sample, in bytes.
    static constexpr auto offsetFinetuneInInstrumentMetadata = sizeSampleTitle + 2; // Offset of the "finetune" inside the Instrument metadata.
    static constexpr auto offsetVolumeInInstrumentMetadata = sizeSampleTitle + 3;   // Offset of the "volume" inside the Instrument metadata.
    static constexpr auto offsetRepeatOffsetInInstrumentMetadata = sizeSampleTitle + 4;   // Offset of the "offset repeat" inside the Instrument metadata.
    static constexpr auto offsetRepeatLengthInInstrumentMetadata = sizeSampleTitle + 6;   // Offset of the "length repeat" inside the Instrument metadata.
    static constexpr auto sizeLinkerData = 1 + 1 + 128;  // Size of the block where the linker is.
    static constexpr auto mkMarkerSize = 4;                // Size of the "M.K." mark.
    static constexpr auto offsetMKMark31Samples = offsetSamplesMetadata + (31 * sizeSampleMetadata) + sizeLinkerData;     // "M.K." mark (or other!) offset, if 31 samples.
    static constexpr auto maximumEndOffsetOfMK = offsetMKMark31Samples + (mkMarkerSize - 1);   // Highest "M.K" mark (or other!) offset, last offset.

    static constexpr auto linesPerPattern = 64;       // How many lines per Pattern. Never changes!
    static constexpr auto sizePatternCell = 4;        // Size of one cell of one Track of a Pattern (note, effect).

    static const double pitchMultiplier;

    /**
     * Finds the channel count from the given stream. It is reset at the end, so that
     * the full parsing can be done.
     * This method is useful to know how many channels the MOD has, before fully parsing it.
     * @param inputStream a valid InputStream.
     * @param isExtensionMod true if the extension is "mod".
     * @return the channel count, or 0 if it couldn't be found.
     */
    static int findChannelCount(juce::InputStream& inputStream, bool isExtensionMod) noexcept;

    /**
     * Finds how many channels and instruments the module has. If it couldn't be found, 0 is returned.
     * @param memoryBlock a MemoryBlock. It may not have enough data to read the "M.K." mark (or any other mark).
     * @param isExtensionMod true if the extension is "mod".
     * @return the channel and instrument count, or 0 if they couldn't be found (wrong format).
     */
    static std::pair<int, int> findChannelAndInstrumentCount(const juce::MemoryBlock& memoryBlock, bool isExtensionMod) noexcept;

    /**
     * Reads the marker at the specific location, and returns how channel the module has from it.
     * @param memoryBlock a MemoryBlock, with at least enough data to read the "M.K." mark (or any other mark).
     * @param offsetMarker the offset to the marker.
     * @return the channel count, or 0 if it couldn't be found (no marker here, wrong format).
     */
    static unsigned int readChannelCount(const juce::MemoryBlock& memoryBlock, int offsetMarker) noexcept;

    /**
     * Parses the Module, which data are already fully loaded in the given MemoryBlock, to be read with the getSong method.
     * @param memoryBlock the data of the song.
     * @param modConfiguration the import configuration.
     */
    void parse(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept;

    /** Reads and stores the Pattern indexes. Also stores the highest Pattern index. This must be done prior to reading the Instruments. */
    bool readPatternIndexes(const juce::MemoryBlock& memoryBlock) noexcept;

    /**
     * Reads all the instruments and put them in the song. The parsing of the Pattern Linker MUST have been performed before!
     * @param memoryBlock the data of the song.
     * @param modConfiguration the import configuration.
     * @return true if no critical error was found.
     */
    bool readAndCreateAllInstruments(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept;

    /**
     * Reads one instrument and put it in the song.
     * @param memoryBlock the data of the song.
     * @param instrumentIndex the instrument index to read (from 0 to 30 included).
     * @param modConfiguration the import configuration.
     * @return true if no critical error was found.
     */
    bool readAndCreateInstrument(const juce::MemoryBlock& memoryBlock, int instrumentIndex, const ModConfiguration& modConfiguration) noexcept;

    /**
     *  Builds a table indicating, for each instrument, its offset. Required before creating a specific Instrument.
     *  @param memoryBlock the data of the song.
     */
    bool buildSampleOffsetTable(const juce::MemoryBlock& memoryBlock) noexcept;

    /**
     * Retrieves the length of a sample, for the Instrument which index is given (0-30).
     * @param memoryBlock the data of the song.
     * @param instrumentIndex the instrument index to read (from 0 to 30 included).
     * @return the size, or -1 if an error occurred. A size of 0 or 1 means "no sample".
     */
    int retrieveSampleLength(const juce::MemoryBlock& memoryBlock, int instrumentIndex) const noexcept;

    /** Builds a generic PSG instrument. */
    void buildGenericPsgInstrument(int instrumentIndex, const juce::String& title, bool isLooping) const noexcept;

    /**
     * Populates the current Subsong.
     * @param memoryBlock the data of the song.
     * @param modConfiguration the import configuration.
     */
    void populateSubsong(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept;

    /**
     * Reads the Patterns and adds them, along with the related Tracks, if necessary.
     * @param memoryBlock the data of the song.
     * @param modConfiguration the import configuration.
     */
    void readAndCreatePatternsAndTracks(const juce::MemoryBlock& memoryBlock, const ModConfiguration& modConfiguration) noexcept;

    /** Declares the Positions, from the already read Patterns. */
    void declarePositions() noexcept;

    /**
     * Extracts the MOD full pattern from the song.
     * @param songMemoryBlock the data of the song.
     * @param patternIndex the index of the Pattern.
     * @return the MemoryBlock of the extracted Pattern.
     */
    std::unique_ptr<juce::MemoryBlock> extractFullPattern(const juce::MemoryBlock& songMemoryBlock, int patternIndex) noexcept;

    /**
     * Generates a Pattern (in the MOD format) inside which only the desired Tracks are present, and some are mixed.
     * @param patternMemoryBlock the pattern of the song.
     * @param keptTrackIndexes the indexes of the Tracks to keep.
     * @param trackIndexesToMix the tracks to mix (source, destination)*.
     * @return the MemoryBlock of the generated Pattern, or nullptr if an error occurred. The Pattern will have a multiple of 3 track count.
     */
    std::unique_ptr<juce::MemoryBlock> generatePatternFromConfiguration(const juce::MemoryBlock& patternMemoryBlock, const std::set<int>& keptTrackIndexes,
                                                                  const std::vector<std::pair<int, int>>& trackIndexesToMix) const noexcept;

    /**
     * Reads the given Pattern data, extracts its Tracks and encodes them. Also declares the Pattern for the current Subsong.
     * @param patternMemoryBlock the raw of data of the Pattern.
     * @param patternIndex the index of the Pattern.
     * @param isImportInstrumentsAsPsg true to import as PSG instrument, not samples.
     * @param contexts the Context, one per *output* channel.
     */
    void readPatternBlockAndCreateTracksAndDeclarePattern(const juce::MemoryBlock& patternMemoryBlock, int patternIndex,
                                                          bool isImportInstrumentsAsPsg, std::vector<TrailingEffectContext>& contexts) noexcept;

    /**
     * Encodes a Module Cell. If empty, nothing happens.
     * @param lineIndex the index if the line to encode.
     * @param modCell the cell, not empty.
     * @param trackIndex the index of the current Track. Only useful for the status report.
     * @param isImportInstrumentsAsPsg true to import as PSG instrument, not samples.
     * @param context the track context.
     */
    void encodeCell(int lineIndex, const ModTrackCell& modCell, int trackIndex, bool isImportInstrumentsAsPsg, TrailingEffectContext& context) noexcept;

    /**
     * Mixes two Tracks from one original Pattern. The Pattern is modified.
     * @param patternMemoryBlock the Pattern to read.
     * @param channelCount how many channels there are.
     * @param trackIndexToMix source and destination Track indexes.
     */
    static void mixTracks(juce::MemoryBlock& patternMemoryBlock, int channelCount, const std::pair<int, int>& trackIndexToMix) noexcept;

    /**
     * Generates a Pattern in the MOD format with the desired channel count, from the input Pattern, keeping only certain Tracks.
     * @param inputPatternMemoryBlock the input Pattern, in the MOD format.
     * @param inputChannelCount the channel count of the input Pattern.
     * @param outputChannelCount the channel count of the Pattern to generate.
     * @param keptTrackIndexes the indexes of the Tracks to keep.
     * @return the generated Pattern, or nullptr if an error occurred.
     */
    static std::unique_ptr<juce::MemoryBlock> generateOutputPatternWithSpecificTracks(const juce::MemoryBlock& inputPatternMemoryBlock, int inputChannelCount,
                                                                                int outputChannelCount, const std::set<int>& keptTrackIndexes) noexcept;

    /**
     * Finds the note related to the given period. If the period does not exactly match the "official" periods, the higher one is chosen.
     * @param period the period. The note returned is from 0 to 4*12 only.
     * @return the note (>= 0), in the MOD range.
     */
    int findModuleNoteIndexFromPeriod(int period) noexcept;

    /**
     * Reads the given Pattern, generates the Speed Track accordingly (if needed) and sets the (internal value) height of the Pattern.
     * @param patternMemoryBlock the raw data of the Pattern.
     * @param inputPatternChannelCount how many channels are in the given Pattern.
     * @param patternIndex the Pattern index.
     */
    void readPatternGenerateSpeedTrackFindHeightsAndLoop(const juce::MemoryBlock& patternMemoryBlock, int inputPatternChannelCount, int patternIndex) noexcept;

    /**
     * @return a Cell from a MOD Pattern.
     * @param patternData the raw MOD Pattern.
     * @param patternChannelCount how many channels are inside the Pattern (>=4).
     * @param channelIndex the channel to extract the Cell from (>=0).
     * @param lineIndex the line to extract the Cell from (>=0).
     */
    static ModTrackCell readCell(const juce::MemoryBlock& patternData, int patternChannelCount, int channelIndex, int lineIndex) noexcept;

    // Effects.
    // =======================================

    /**
     * Manages the effects from the given cell.
     * @param modCell the cell.
     * @param lineIndex the line index (>=0, <=127).
     * @param isNote true if a note is present.
     * @param instrumentNumber if there is a note, the Instrument Number (>0).
     * @param context the Track Context.
     * @param cell the Cell which effects must be filled. It has none.
     */
    void readAndManageEffects(const ModTrackCell& modCell, int lineIndex, bool isNote, int instrumentNumber, TrailingEffectContext& context, Cell& cell) noexcept;

    /**
     * Manages the speed effect.
     * @param patternIndex the pattern index.
     * @param effectValue the value, from 0 to 255.
     * @param lineIndex the line index (>=0, <=127).
     */
    void manageSpeedEffect(int patternIndex, int effectValue, int lineIndex) const noexcept;

    /**
     * Adds an effect, if different, to the given Cell.
     * @param trailingEffect the trailingEffect effect to look for.
     * @param outputEffect effect the effect to encode.
     * @param value the value.
     * @param context the Context of the Track.
     * @param cell the Cell to add the effect to.
     */
    static void addEffectFromContext(TrailingEffect trailingEffect, Effect outputEffect, int value, TrailingEffectContext& context, Cell& cell) noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.

    int originalSongChannelCount;                   // The channel count of the *original* song (4, 6, 8 etc.). If 0, problem or uninitialized.
    int instrumentCount;                            // 31 instruments (1-31), or 15 (1-15) for old modules, pre-M.K.).
    int outputSongChannelCount;                     // The channel count of the *output* song (3, 6, 9 etc.). If 0, problem or uninitialized.
    int offsetPatternLinker;                        // Offset of the Pattern linker (position count, loop (ignored), Pattern indexes).
    int offsetPatternsData;                         // Offset of the first Pattern.

    int originalSizePattern;                        // The size of one Pattern of the *original* song. Depends on the original channel count. If 0, problem or uninitialized.
    std::vector<unsigned char> positionToPatternIndexes;      // The indexes of all the Patterns. The size shows the song length.
    unsigned char highestPatternIndex;              // The highest index of the Pattern. All of these are encoded.

    std::vector<int> sampleOffsets;                 // The offset for each sample data. Even empty sample should be there.
    std::unordered_map<int, unsigned char> sampleIndexToVolume;     // Links a sample index (>0!) to its volume from 0 to 63(!).

    std::unordered_map<int, int> notePeriodToNoteNumber;    // Links periods found in a module to its note number. Built on the fly for a faster look-up.

    std::unordered_map<int, int> patternIndexToHeight;      // Links a Pattern index to a height.
    OptionalInt foundLoopTo;                                // The loop to.
    OptionalInt foundEnd;                                   // Index of the position which is the last. Found via a backward Position Jump effect.
};

}   // namespace arkostracker
