#pragma once

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../SongImporter.h"

namespace arkostracker 
{

/** Imports an AT2 song. */
class At2SongImporter final : public SongImporter
{
public:
    /** Constructor. */
    At2SongImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

    // =======================================

    /**
     * @return a link from the String, or absent if it could not be parsed.
     * @param inputString the input String.
     */
    static OptionalValue<PsgInstrumentCellLink> parseLinkFromString(const juce::String& inputString) noexcept;

private:
    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param songNode the node of the Song.
     */
    void parse(const juce::XmlElement& songNode) noexcept;

    /**
     * @return an epoch (ms) from a date in the format YYYY-MM-DD. The separator don't matter.
     * If unable to do so, returns the current date.
     * @param date the date, as a String.
     */
    static juce::int64 convertDateToEpoch(const juce::String& date) noexcept;

    /**
     * Parses the PSG Instruments.
     * @param songNode the node of the Song.
     */
    void parsePsgInstruments(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Cell of a Psg Instrument node.
     * @param psgInstrumentCellNode the node of the Cell.
     */
    void parsePsgInstrumentCell(const juce::XmlElement& psgInstrumentCellNode) noexcept;

    /**
     * Parses the sample Instruments.
     * @param songNode the node of the Song.
     */
    void parseSampleInstruments(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Arpeggios.
     * @param songNode the node of the Song.
     */
    void parseArpeggios(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Pitches.
     * @param songNode the node of the Song.
     */
    void parsePitches(const juce::XmlElement& songNode) noexcept;

    /**
     * Parses the Subsongs.
     * @param songNode the node of the Song.
     */
    void parseSubsongs(const juce::XmlElement& songNode) noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
};

}   // namespace arkostracker
