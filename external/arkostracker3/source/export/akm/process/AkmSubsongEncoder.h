#pragma once

#include <juce_core/juce_core.h>

#include "../../../utils/Id.h"
#include "../../../song/tracks/Track.h"

namespace arkostracker 
{

class CellEffects;
class ErrorReport;
class ExportConfiguration;
class PlayerConfiguration;
class Song;
class SongExportResult;
class SourceGenerator;

/** Encodes one Subsong in the AKM format. */
class AkmSubsongEncoder
{
public:
    /**
     * Constructor.
     * @param song the song to export. It should be optimized, but this is not mandatory.
     * Important: the Legato Effects MUST have generated before!
     * @param subsongId the ID of the Subsong.
     * @param exportConfiguration the export configuration.
     * @param songExportResult the result of the export.
     * @param baseLabelForSubsong the label to put before each generated label (example: "MySong_"), for this Subsong. It should have a separator if wanted.
     */
    AkmSubsongEncoder(const Song& song,
                       Id subsongId,
                       const ExportConfiguration& exportConfiguration,
                       SongExportResult& songExportResult,
                       juce::String baseLabelForSubsong) noexcept;

    /** Exports the Subsong data into the OutputStream. Only the part specific to the Subsong are generated (Linker, Tracks, SpecialTracks). */
    void generateSubsong() noexcept;

private:
    /** Parses the Song. This is the first step before actually encoding. */
    void parseSubsongAndBuildStructures() noexcept;

    /**
     * For more optimization, finds default note/instrument/wait at the beginning of each Tracks, keep the best,
     * but they should be different from the primary/secondary instruments/waits, else it defeats the purpose.
     * Thus, the primary/secondary instruments/waits must have been set.
     * Updates some internal values.
     */
    void findDefaultTrackStartItems() noexcept;

    /** Encodes the Header of a Subsong. */
    void encodeHeader(SourceGenerator& sourceGenerator) const noexcept;
    /** Encodes the Linker. */
    void encodeLinker(SourceGenerator& sourceGenerator) const noexcept;

    /** @return the label for the table index of the Tracks. */
    juce::String getTrackIndexTable() const noexcept;
    /** @return the label for the table of the note indexes. */
    juce::String getNoteIndexTableLabel() const noexcept;
    /**
     * @return the label for a Track.
     * @param trackIndex the index of the Track.
     */
    juce::String getTrackLabel(int trackIndex) const noexcept;

    /** Encodes the Tracks. The structures must have been built first! */
    void encodeTracks(SourceGenerator& sourceGenerator) const noexcept;

    /**
     * Encodes the given Track.
     * @param sourceGenerator the source generator.
     * @param trackIndex the index of the Track. Useful for its naming.
     * @param track the Track to encode.
     */
    void encodeTrack(SourceGenerator& sourceGenerator, int trackIndex, const Track& track) const noexcept;

    /** Encodes the table with the note indexes. */
    void encodeNoteIndexTable(SourceGenerator& sourceGenerator) const noexcept;

    const Song& song;
    Id subsongId;
    const ExportConfiguration& exportConfiguration;
    SongExportResult& songExportResult;
    juce::String baseLabelForSubsong;

    bool areEffectsPresentInSong;                                           // True if there are effects in the Song.
    int primaryDefaultInstrument;                                           // The most used instrument, calculated from the Song.
    int secondaryDefaultInstrument;                                         // The second most used instrument, calculated from the Song.
    int primaryWait;                                                        // The most used wait, calculated from the Song.
    int secondaryWait;                                                      // The second most used wait, calculated from the Song.
    std::vector<int> mostUsedNoteIndexes;                                   // From most to least used notes.
    int startNoteInTrack;                                                   // A start Note used as default at the beginning of Track.
    int startInstrumentInTrack;                                             // A start Instrument used as default at the beginning of Track.
    int startWaitInTrack;                                                   // A start Wait used as default at the beginning of Track.
    bool foundDefaultNoteInstrumentAndWait;                                 // True when the primary/secondary instrument/notes/wait has been found.
};

}   // namespace arkostracker
