#pragma once

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../SongImporter.h"

namespace arkostracker 
{

/** Imports an AT3 song. */
class At3SongImporter : public SongImporter
{
public:
    /** Constructor. */
    At3SongImporter() noexcept;

    // SongImporter method implementations.
    // =======================================
    bool doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept override;
    std::unique_ptr<Result> loadSong(juce::InputStream& inputStream, const ImportConfiguration& configuration) noexcept override;
    ImportedFormat getFormat() noexcept override;

private:
    /**
     * Parses a song. It will fill the Song and Error Report.
     * @param songNode the node of the Song.
     */
    void parse(const juce::XmlElement& songNode) const noexcept;

    /**
     * Parses the Instruments.
     * @param songNode the node of the Song.
     */
    void parseInstruments(const juce::XmlElement& songNode) const noexcept;

    /**
     * Parses the Expressions.
     * @param isArpeggio true if Arpeggios, false if Pitches.
     * @param songNode the node of the Song.
     */
    void parseExpressions(bool isArpeggio, const juce::XmlElement& songNode) const noexcept;

    /**
     * Parses the subsongs. It will fill the Song and Error Report.
     * @param songNode the node of the Song.
     */
    void parseSubsongs(const juce::XmlElement& songNode) const noexcept;

    /**
     * Parses one subsong. It will fill the Song and Error Report.
     * @param subsongBuilder the Builder of this Subsong.
     * @param subsongNode the node of the Subsong.
     */
    void parseSubsong(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept;

    /**
     * Parses the Tracks. It will fill the Song and Error Report.
     * @param subsongBuilder the Builder of this Subsong.
     * @param subsongNode the node of the Subsong.
     */
    void parseTracks(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept;

    /**
     * Parses the Special Tracks. It will fill the Song and Error Report.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsongBuilder the Builder of this Subsong.
     * @param subsongNode the node of the Subsong.
     */
    void parseSpecialTracks(bool isSpeedTrack, SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept;

    /**
     * Parses the Positions. It will fill the Song and Error Report.
     * @param subsongBuilder the Builder of this Subsong.
     * @param subsongNode the node of the Subsong.
     */
    void parsePositions(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept;

    /**
     * Parses the Patterns. It will fill the Song and Error Report.
     * @param subsongBuilder the Builder of this Subsong.
     * @param subsongNode the node of the Subsong.
     */
    void parsePatterns(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept;

    std::unique_ptr<SongBuilder> songBuilder;                       // To build the Song.
};

}   // namespace arkostracker

