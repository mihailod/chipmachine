#pragma once

/** Simple holder of the flags to know what to encode. */
class EncodedDataFlag
{
public:
    explicit EncodedDataFlag(bool defaultValue = false);

    EncodedDataFlag(bool encodeSongAndSubsongMetadata, bool encodeReferenceTables, bool encodeSpeedTrack, bool encodeEventTrack,
        bool encodeInstruments, bool encodeArpeggioTables, bool encodePitchTables, bool encodeEffects, bool encodeRleForEmptyLines,
        bool encodeTranspositions, bool encodeHeight, double pitchTrackEffectRatio) noexcept;

    bool mustEncodeSongAndSubsongMetadata() const;
    bool mustEncodeReferenceTables() const;
    bool mustEncodeSpeedTrack() const;
    bool mustEncodeEventTrack() const;
    bool mustEncodeInstruments() const;
    bool mustEncodeArpeggioTables() const;
    bool mustEncodePitchTables() const;
    bool mustEncodeEffects() const;
    bool mustEncodeRleForEmptyLines() const;
    bool mustEncodeTranspositions() const;
    bool mustEncodeHeight() const;
    double getPitchTrackEffectRatio() const;

private:
    bool encodeSongAndSubsongMetadata;                // True to encode the Song and Subsong metadata.
    bool encodeReferenceTables;                       // True to encode the reference tables.
    bool encodeSpeedTrack;                            // True to encode the Speed Tracks.
    bool encodeEventTrack;                            // True to encode the Event Tracks.
    bool encodeInstruments;                           // True to encode the Instrument Tracks (Sample Instruments are NOT encoded).
    bool encodeArpeggioTables;                        // True to encode the Arpeggio tables.
    bool encodePitchTables;                           // True to encode the Pitch tables.
    bool encodeEffects;                               // True to encode the effects in the Tracks.
    bool encodeRleForEmptyLines;                      // True to allow empty lines to be RLEd for normal Tracks.
    bool encodeTranspositions;                        // True to encode the transpositions in the Linker.
    bool encodeHeight;                                // True to encode the height in the Linker.

    double pitchTrackEffectRatio;                     // The ratio to which export the Pitch tracks effects.
};
