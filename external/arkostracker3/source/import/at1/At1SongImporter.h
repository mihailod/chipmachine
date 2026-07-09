#pragma once

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../SongImporter.h"

namespace arkostracker 
{

class At1SongImporter : public SongImporter
{
public:
    /** Constructor. */
    At1SongImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    static constexpr auto at1RstNote = 144;                  // The value for an RST note in AT1 format.
    static constexpr auto at1EmptyNote = 145;                // The value for an empty note in AT1 format.
    static constexpr auto at1EmptyVolume = 255;              // The value for an empty volume in AT1 format.
    static constexpr auto at1EmptyPitch = 0;                 // The value for an empty pitch in AT1 format.

    static constexpr auto at1SpecialCellNoEffect = 0;
    static constexpr auto at1SpecialCellEffectSpeed = 1;
    static constexpr auto at1SpecialCellEffectEvent = 2;

    static constexpr auto at1LinkStateHardwareDependentToSoftware = 1;
    static constexpr auto at1LinkStateSoftwareDependentToHardware = 2;

    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param songNode the node of the Song.
     */
    void parse(const juce::XmlElement& songNode) const noexcept;

    /** @return the PSG Frequency, or a default one if unable to extract it. */
    int extractReplayFrequency(const juce::XmlElement& songNode) const noexcept;

    /**
     * Parses the Tracks.
     * @param tracksNode the node of the "TracksArray". Nullptr if not found (error).
     * @param subsongBuilder the builder to fill.
     */
    void parseTracks(const juce::XmlElement* tracksNode, SubsongBuilder& subsongBuilder) const;

    /**
     * Parses the SpecialTracks.
     * @param specialTracksNode the node of the "SpecialTracksArray". Nullptr if not found (error).
     * @param subsongBuilder the builder to fill.
     */
    void parseSpecialTracks(const juce::XmlElement* specialTracksNode, SubsongBuilder& subsongBuilder) const;

    /**
     * Parses the Patterns.
     * @param patternsNode the node of the "PatternsList". Nullptr if not found (error).
     * @param subsongBuilder the builder to fill.
     */
    void parsePatterns(const juce::XmlElement* patternsNode, SubsongBuilder& subsongBuilder) const;

    /**
     * Parses the Instruments.
     * @param instrumentsNode the node of the "InstrumentsList". Nullptr if not found (error).
     */
    void parseInstruments(const juce::XmlElement* instrumentsNode) const;

    void declareInstrumentCell(const juce::XmlElement& cell) const;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
};

}   // namespace arkostracker
