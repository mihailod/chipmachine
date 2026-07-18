#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

class Song;
class Subsong;

/** Exports a Song in the AT3 XML format. */
class SongExporter
{
public:
    /** Prevents instantiation. */
    SongExporter() = delete;

    /**
     * Exports the Song as XML.
     * @param song the Song.
     * @return an XML node, or nullptr if an error occurred (shouldn't happen).
     */
    static std::unique_ptr<juce::XmlElement> exportSong(const Song& song) noexcept;

private:
    /**
     * Encodes the Instruments.
     * @param song the Song to encode.
     * @param songRoot the XML root where to write.
     */
    static void encodeInstruments(const Song& song, juce::XmlElement& songRoot) noexcept;

    /**
     * Encodes the Expressions.
     * @param song the Song to encode.
     * @param songRoot the XML root where to write.
     * @param isArpeggio true to encode the Arpeggio, false to encode the Pitches.
     */
    static void encodeExpressions(const Song& song, juce::XmlElement& songRoot, bool isArpeggio) noexcept;

    /**
     * Encodes a Subsong.
     * @param subsong the Subsong to encode.
     * @param subsongsNode the XML root where to write.
     */
    static void encodeSubsong(const Subsong& subsong, juce::XmlElement& subsongsNode) noexcept;

    /**
     * Encodes the Tracks.
     * @param subsong the Subsong to encode.
     * @param subsongNode the XML root where to write.
     */
    static void encodeTracks(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept;

    /**
     * Encodes the Tracks.
     * @param subsong the Subsong to encode.
     * @param subsongNode the XML root where to write.
     * @param isSpeedTrack true to encode speed track, false for event track.
     */
    static void encodeSpecialTracks(const Subsong& subsong, juce::XmlElement& subsongNode, bool isSpeedTrack) noexcept;

    /**
     * Encodes the Positions.
     * @param subsong the Subsong to encode.
     * @param subsongNode the XML root where to write.
     */
    static void encodePositions(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept;

    /**
     * Encodes the Patterns.
     * @param subsong the Subsong to encode.
     * @param subsongNode the XML root where to write.
     */
    static void encodePatterns(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept;
};

}   // namespace arkostracker
