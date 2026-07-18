#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

class SourceGenerator;
class Song;
class ExportConfiguration;
class SongExportResult;

/** Class that generates AKM data from the Song (not Subsongs). */
class AkmSongEncoder
{
public:
    /**
     * Constructor.
     * @param song the song to export. It should be optimized, but this is not mandatory. Important: the Legato Effects MUST have generated before!
     * @param exportConfiguration the export configuration.
     * @param songExportResult object to fill with the result.
     * @param baseLabel a base label.
     * @param getSubsongLabel a function to get a label for a Subsong.
     */
    AkmSongEncoder(const Song& song, const ExportConfiguration& exportConfiguration, SongExportResult& songExportResult, juce::String baseLabel,
                   std::function<juce::String(int subsongIndex)> getSubsongLabel) noexcept;

    /** Encodes the Song. */
    void encodeSong() const noexcept;

private:
    /** Encodes the Song header. */
    void encodeSongHeader(SourceGenerator& sourceGenerator) const noexcept;

    /** Encodes the Instrument table and the Instruments themselves. */
    void encodeInstrumentsAndIndex(SourceGenerator& sourceGenerator) const noexcept;

    /** Encodes the expressions and their tables. */
    void encodeExpressionsAndIndexes(SourceGenerator& sourceGenerator) const noexcept;

    /** Encodes expressions and indexes tables. */
    void encodeExpressionsAndIndexes(SourceGenerator& sourceGenerator, bool isArpeggio, const std::function<juce::String()>& getExpressionTableLabel,
                                     const std::function<juce::String(int index)>& getExpressionLabel) const noexcept;

    /** @return the label for an Instrument, which index is given. */
    juce::String getInstrumentLabel(int instrumentIndex) const noexcept;
    /** @return the label for the table index of the Instrument. */
    juce::String getInstrumentIndexLabel() const noexcept;
    /** @return the label for the table index of the Arpeggios. */
    juce::String getArpeggioIndexLabel() const noexcept;
    /** @return the label for the table index of the Pitches. */
    juce::String getPitchIndexLabel() const noexcept;
    /** @return the label for the an Arpeggio, which index is given. */
    juce::String getArpeggioLabel(int index) const noexcept;
    /** @return the label for the a Pitch, which index is given. */
    juce::String getPitchLabel(int index) const noexcept;

    const Song& song;
    const ExportConfiguration& exportConfiguration;
    SongExportResult& songExportResult;
    juce::String baseLabel;
    std::function<juce::String(int subsongIndex)> getSubsongLabel;
};

}   // namespace arkostracker
