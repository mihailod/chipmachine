#include "EncodedDataFlag.h"

EncodedDataFlag::EncodedDataFlag(const bool defaultValue) :
        encodeSongAndSubsongMetadata(defaultValue),
        encodeReferenceTables(defaultValue),
        encodeSpeedTrack(defaultValue),
        encodeEventTrack(defaultValue),
        encodeInstruments(defaultValue),
        encodeArpeggioTables(defaultValue),
        encodePitchTables(defaultValue),
        encodeEffects(defaultValue),
        encodeRleForEmptyLines(defaultValue),
        encodeTranspositions(defaultValue),
        encodeHeight(defaultValue),
        pitchTrackEffectRatio(1.0)
{
}

EncodedDataFlag::EncodedDataFlag(const bool pEncodeSongAndSubsongMetadata, const bool pEncodeReferenceTables, const bool pEncodeSpeedTrack, const bool pEncodeEventTrack,
        const bool pEncodeInstruments, const bool pEncodeArpeggioTables, const bool pEncodePitchTables, const bool pEncodeEffects, const bool pEncodeRleForEmptyLines,
        const bool pEncodeTranspositions, const bool pEncodeHeight, const double pPitchTrackEffectRatio) noexcept :
        encodeSongAndSubsongMetadata(pEncodeSongAndSubsongMetadata),
        encodeReferenceTables(pEncodeReferenceTables),
        encodeSpeedTrack(pEncodeSpeedTrack),
        encodeEventTrack(pEncodeEventTrack),
        encodeInstruments(pEncodeInstruments),
        encodeArpeggioTables(pEncodeArpeggioTables),
        encodePitchTables(pEncodePitchTables),
        encodeEffects(pEncodeEffects),
        encodeRleForEmptyLines(pEncodeRleForEmptyLines),
        encodeTranspositions(pEncodeTranspositions),
        encodeHeight(pEncodeHeight),
        pitchTrackEffectRatio(pPitchTrackEffectRatio)
{
}

bool EncodedDataFlag::mustEncodeSongAndSubsongMetadata() const
{
    return encodeSongAndSubsongMetadata;
}

bool EncodedDataFlag::mustEncodeReferenceTables() const
{
    return encodeReferenceTables;
}

bool EncodedDataFlag::mustEncodeSpeedTrack() const
{
    return encodeSpeedTrack;
}

bool EncodedDataFlag::mustEncodeEventTrack() const
{
    return encodeEventTrack;
}

bool EncodedDataFlag::mustEncodeInstruments() const
{
    return encodeInstruments;
}

bool EncodedDataFlag::mustEncodeArpeggioTables() const
{
    return encodeArpeggioTables;
}

bool EncodedDataFlag::mustEncodePitchTables() const
{
    return encodePitchTables;
}

bool EncodedDataFlag::mustEncodeEffects() const
{
    return encodeEffects;
}

bool EncodedDataFlag::mustEncodeRleForEmptyLines() const
{
    return encodeRleForEmptyLines;
}

bool EncodedDataFlag::mustEncodeTranspositions() const
{
    return encodeTranspositions;
}

bool EncodedDataFlag::mustEncodeHeight() const
{
    return encodeHeight;
}

double EncodedDataFlag::getPitchTrackEffectRatio() const
{
    return pitchTrackEffectRatio;
}
