#pragma once

#include "juce_core/juce_core.h"

#include "../../../utils/Id.h"
#include "../../../song/cells/Note.h"

namespace arkostracker 
{

class CellEffects;
class ErrorReport;
class ExportConfiguration;
class PlayerConfiguration;
class Song;
class SongExportResult;
class SourceGenerator;

/**	Class that is used by AkgSongExporter, that generates the Subsongs. */
class AkgSubsongExporter
{
public:
    /**
     * Constructor.
     * @param song the song to export. It should be optimized, but this is not mandatory.
     * Important: the Legato Effects MUST have generated before!
     * @param subsongId the ID of the Subsong.
     * @param errorReport the report to fill.
     * @param baseLabelForSubsong the label to put before each generated label (example: "MySong_"), for this Subsong. It should have a separator if wanted.
     * @param effectBlockIdToIndex map linking a block ID to its index, if any. A copy is performed.
     * @param effectBlockIdToOffsetFromEffectBlockTable links the ids of all Effect Blocks to their offset, in bytes, from the Effect Block table.
     */
    AkgSubsongExporter(const Song& song,
                       Id subsongId,
                       const ExportConfiguration& exportConfiguration,
                       PlayerConfiguration& playerConfiguration,
                       SongExportResult& songExportResult,
                       juce::String baseLabelForSubsong,
                       const std::map<juce::String, int>& effectBlockIdToIndex,
                       const std::map<juce::String, int>& effectBlockIdToOffsetFromEffectBlockTable) noexcept;

    /** Exports the Subsong data into the OutputStream. Only the part specific to the Subsong are generated (Linker, Tracks, SpecialTracks). */
    void generateSubsong() noexcept;

private:
    static const juce::String linkerBlockLabelString;   // Label for one LinkerBlock.
    static const juce::String trackLabelString;         // Label for one Track.
    static const juce::String speedTrackLabelString;    // Label for one SpeedTrack.
    static const juce::String eventTrackLabelString;    // Label for one EventTrack.

    static const int opcodeNoNoteMaybeEffects = 60;     // Opcode to encode a no note, but maybe effects. Used to encode a wait of 1.
    static const int opcodeWaitDuringXLines = 61;       // Opcode to encode a >5 empty lines.
    static const int opcodeWaitDuring2To5Lines = 62;    // Opcode to encode from 2 to 5 empty lines.
    static const int opcodeEscapeNote = 63;             // Opcode to encode an escape note.
    static const int optimizedNoteRange = 56;			// How large is the optimized notes range, including all the optimized notes.

    /**
     * Converts the given frequency to an index such as used by players. If unknown, acts as if it was 50hz.
     * @param frequency the frequency in Hz (12.5, 25, 50, etc.).
     * @return the the frequency index (0=13hz, 1=25, 2=50... 5=300).
     */
    static int convertHzToPlayerFrequencyIndex(float frequency) noexcept;

    /**
     * Encodes the Subsong Linker into the OutputStream.
     * @param sourceGenerator the source generator where to declare the data.
     */
    void encodeSubsongLinker(SourceGenerator& sourceGenerator) noexcept;

    /**
     * @return the label for a Track, for a specific Subsong.
     * @param trackIndex the index of the Track.
     */
    juce::String getTrackLabel(int trackIndex) const noexcept;

    /**
     * @return the label for a LinkerBlock, for a specific Subsong.
     * @param linkerBlockIndex the index of the LinkerBlock.
     */
    juce::String getLinkerBlockLabel(int linkerBlockIndex) const noexcept;

    /**
     * @return the label for a SpeedTrack, for a specific Subsong.
     * @param speedTrackIndex the index of the SpeedTrack.
     */
    juce::String getSpeedTrackLabel(int speedTrackIndex) const noexcept;

    /**
     * @return the label for a EventTrack, for a specific Subsong.
     * @param eventTrackIndex the index of the SpeedTrack.
     */
    juce::String getEventTrackLabel(int eventTrackIndex) const noexcept;

    /**
     * Encodes the Subsong Tracks into the OutputStream.
     * @param sourceGenerator the source generator where to declare the data.
     * @param baseNoteIndex the base note index.
     */
    void encodeTracks(SourceGenerator& sourceGenerator, int baseNoteIndex) const noexcept;

    /**
     * Encodes the Subsong Tracks into the OutputStream.
     * @param trackIndex the index of the Track to encode. It must be valid.
     * @param sourceGenerator the source generator where to declare the data.
     * @param baseNoteIndex the base note index.
     */
    void encodeTrack(int trackIndex, SourceGenerator& sourceGenerator, int baseNoteIndex) const noexcept;

    /**
     * Encodes empty cells. If the cell count is 0, nothing happens.
     * @param cellCount how many cells to encode. May be 0.
     * @param sourceGenerator the source generator where to declare the data.
     */
    static void encodeEmptyCells(int cellCount, SourceGenerator& sourceGenerator) noexcept;

    /**
     * Encodes a reference to an effect block, and stores the EffectBlock under a reference for a later encoding.
     * @param note the possible note. Useful by some effects, such as Glide. Ignored anyway.
     * @param cellEffects the effects. May be empty. It DOES hold the possible Legato.
     * @param sourceGenerator the source generator where to declare the data.
     */
    void encodeEffectBlockReferenceAndStoreEffects(OptionalValue<Note> note, const CellEffects& cellEffects, SourceGenerator& sourceGenerator) const noexcept;

    /**
     * Encodes a Special Track, as they are encoded the same way.
     * @param sourceGenerator the source generator where to declare the data.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     */
    void encodeSpecialTrack(SourceGenerator& sourceGenerator, bool isSpeedTrack) const noexcept;

    /**
     * Encodes a 'wait' for a Special Track, or do nothing if there is nothing to encode.
     * @param sourceGenerator the source generator where to declare the data.
     * @param waitCount how many lines to wait. Should be >=1, but as a convenience, if <=0, nothing happens.
     */
    static void encodeWaitEventForSpecialTrack(SourceGenerator& sourceGenerator, int waitCount) noexcept;

    const Song& song;
    const Id subsongId;
    const ExportConfiguration& exportConfiguration;
    PlayerConfiguration& playerConfiguration;
    SongExportResult& exportResult;
    juce::String baseLabel;
    const std::map<juce::String, int>& effectBlockIdToIndex;
    const std::map<juce::String, int>& effectBlockIdToOffsetFromEffectBlockTable;

    std::set<int> usedTrackIndexes;                                         // The Track indexes to encode.
    std::set<int> usedSpeedTrackIndexes;                                    // The SpeedTrack indexes to encode.
    std::set<int> usedEventTrackIndexes;                                    // The EventTrack indexes to encode.
};

}   // namespace arkostracker

