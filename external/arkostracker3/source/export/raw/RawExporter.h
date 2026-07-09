#pragma once

#include <juce_core/juce_core.h>

#include <functional>

#include "../../utils/task/Task.h"
#include "../ExportConfiguration.h"
#include "../SongExportResult.h"
#include "../sourceGenerator/SourceGenerator.h"
#include "../model/EncodedDataFlag.h"

namespace arkostracker
{

class Instrument;
class Expression;
class Song;

/**
 * Export to Raw format, a non-optimized format containing all the data. This is ideal as a pivot format for conversion,
 * but also useful for MOD players that require a simple format regardless of the wasted size.
 */
class RawExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param song the Song. No need to optimize it.
     * @param encodedDataFlag flags to know what to encode.
     * @param exportConfiguration data about how to export.
     */
    RawExporter(const Song& song, const EncodedDataFlag& encodedDataFlag, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    static constexpr auto noteValueNoNoteButEffects = 120;                   // Value used to mark a note as "not present, but there are effects".

    /**
     * @return the base label to be used before every label.
     * @param showSubsongIndex adds the subsong index after it.
     */
    juce::String getBaseLabel(bool showSubsongIndex = true) const noexcept;

    /** @return the label used by the Linker. */
    juce::String getLinkerLabel() const noexcept;
    /** @return the label used by the Linker, for the loop. */
    juce::String getLinkerLoopLabel() const noexcept;
    /** @return the label of the table of the Track indexes. */
    juce::String getTrackIndexesLabel() const noexcept;
    /** @return the label of the table of the Speed Track indexes. */
    juce::String getSpeedTrackIndexesLabel() const noexcept;
    /** @return the label of the table of the Event Track indexes. */
    juce::String getEventTrackIndexesLabel() const noexcept;
    /** @return the label of the table of the Instrument indexes. */
    juce::String getInstrumentIndexesLabel() const noexcept;
    /** @return the label of the table of the Arpeggios indexes. */
    juce::String getArpeggioIndexesLabel() const noexcept;
    /** @return the label of the table of the Pitches indexes. */
    juce::String getPitchIndexesLabel() const noexcept;

    /** @return the label of a Track. */
    juce::String getTrackLabel(int index) const noexcept;
    /** @return the label of a SpeedTrack. */
    juce::String getSpeedTrackLabel(int index) const noexcept;
    /** @return the label of a EventTrack. */
    juce::String getEventTrackLabel(int index) const noexcept;

    /** @return the label of an Instrument. */
    juce::String getInstrumentLabel(int index) const noexcept;

    /** @return the label of an Arpeggio. */
    juce::String getArpeggioLabel(int index) const noexcept;
    /** @return the label of a Pitch. */
    juce::String getPitchLabel(int index) const noexcept;

    /**
     * @return the label of a SpecialTrack.
     * @param index the index of the Track.
     * @param isSpeedTrack true if SpeedTrack, false if EventTrack.
     */
    juce::String getSpecialTrackLabel(int index, bool isSpeedTrack) const noexcept;

    /** Encodes the header. */
    void encodeHeader(SourceGenerator& sourceGenerator) const noexcept;
    /** Encodes the Linker. Also fills the indexes of the used Tracks (normal, Speed, Event). */
    void encodeLinker(SourceGenerator& sourceGenerator) noexcept;
    /** Encodes the Tracks. The Linker must have been encoded first. */
    void encodeTracks(SourceGenerator& sourceGenerator) const noexcept;
    /**
     * Encodes the SpecialTracks. The Linker must have been encoded first.
     * @param sourceGenerator the source generator.
     * @param isSpeedTrack true if SpeedTrack, false if EventTrack.
     */
    void encodeSpecialTracks(SourceGenerator& sourceGenerator, bool isSpeedTrack) const noexcept;

    /**
     * Encodes empty Cells, if any. If the RLE flag allows it, a RLE sequence is encoded, else the raw cells.
     * @param sourceGenerator the source generator.
     * @param consecutiveEmptyCells how many empty cells there are. May be 0.
     */
    void encodeEmptyCells(SourceGenerator& sourceGenerator, int consecutiveEmptyCells) const noexcept;

    /**
     *  Encodes an effect.
     * @param sourceGenerator the source generator.
     *  @param effectNumber the effet number.
     *  @param effectValue the effet value.
     */
    static void encodeEffect(SourceGenerator& sourceGenerator, int effectNumber, int effectValue) noexcept;

    /** Encodes the Expressions. */
    bool encodeExpressions(SourceGenerator& sourceGenerator, bool isArpeggio) const noexcept;

    /**
     * Encodes an expression.
     * @param sourceGenerator the source generator.
     * @param expression the Expression to encode.
     * @param isArpeggio true if Arpeggio, false is Pitch.
     */
    static void encodeExpression(SourceGenerator& sourceGenerator, const Expression& expression, bool isArpeggio) noexcept;

    /**
     * Generic method to generate index tables. Unused indexes are encoded as "0".
     * @param sourceGenerator the source generator.
     * @param firstComment a comment to display at the beginning of the table.
     * @param firstLabel the label of the table.
     * @param count how many items there are (>0).
     * @param indexToLabelFunction the function to call to get the name of the label, from its index.
     */
    static void encodeIndexTable(SourceGenerator& sourceGenerator, const juce::String& firstComment, const juce::String& firstLabel, int count,
        const std::function<juce::String(int)>& indexToLabelFunction) noexcept;

    /**
     * Encodes the Instruments.
     * @param sourceGenerator the source generator.
     * @return false if the samples are too big.
     */
    bool encodeInstruments(SourceGenerator& sourceGenerator) const noexcept;

    /**
     * Encodes an Instrument.
     * @param sourceGenerator the source generator.
     * @param instrumentIndex the index of the Instrument.
     * @param instrument the PSG Instrument.
     */
    void encodePsgInstrument(SourceGenerator& sourceGenerator, int instrumentIndex, const Instrument& instrument) const noexcept;

    /**
     * Encodes an Sample Instrument.
     * @param sourceGenerator the source generator.
     * @param instrumentIndex the index of the Instrument.
     * @param instrument the sample Instrument.
     * @return true if everything went fine. False if the sample was too big (>65535).
     */
    bool encodeSampleInstrument(SourceGenerator& sourceGenerator, int instrumentIndex, const Instrument& instrument) const noexcept;

    const Song& originalSong;                                                  // The original Song, not stripped/optimized.
    std::unique_ptr<Song> song;
    Id originalSubsongId;
    Id subsongId;
    EncodedDataFlag encodedDataFlag;
    ExportConfiguration exportConfiguration;

    int subsongIndex;
    int maximumHeight;                                                          // The maximum height found in the Linker.
    int trackCount;
    int speedTrackCount;
    int eventTrackCount;
};

}   // namespace arkostracker
