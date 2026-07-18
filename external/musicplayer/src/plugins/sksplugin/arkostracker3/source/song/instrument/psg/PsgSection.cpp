#include "PsgSection.h"

namespace arkostracker 
{

const juce::String PsgSectionHelper::tagSoundType = "soundType";                                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagEnvelope = "envelope";                                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagNoise = "noise";                                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagPrimaryPeriod = "primaryPeriod";                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagPrimaryArpeggioNoteInOctave = "primaryArpeggioNoteInOctave";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagPrimaryArpeggioOctave = "primaryArpeggioOctave";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagPrimaryPitch = "primaryPitch";                                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSecondaryPeriod = "secondaryPeriod";                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSecondaryArpeggioNoteInOctave = "secondaryArpeggioNoteInOctave";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSecondaryArpeggioOctave = "secondaryArpeggioOctave";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSecondaryPitch = "secondaryPitch";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSidBalancePercent = "sidBalancePercent";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSidLowShelfVolume = "sidLowShelfVolume";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSidIsActivated = "sidIsActivated";                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSidArpeggio = "sidArpeggio";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PsgSectionHelper::tagSidPitch = "sidPitch";                                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

juce::String PsgSectionHelper::toString(const PsgSection psgSection) noexcept
{
    switch (psgSection) {
        default:
        case PsgSection::count:
            jassertfalse;           // Abnormal!
        case PsgSection::soundType:
            return tagSoundType;
        case PsgSection::envelope:
            return tagEnvelope;
        case PsgSection::noise:
            return tagNoise;
        case PsgSection::primaryPeriod:
            return tagPrimaryPeriod;
        case PsgSection::primaryArpeggioNoteInOctave:
            return tagPrimaryArpeggioNoteInOctave;
        case PsgSection::primaryArpeggioOctave:
            return tagPrimaryArpeggioOctave;
        case PsgSection::primaryPitch:
            return tagPrimaryPitch;
        case PsgSection::secondaryPeriod:
            return tagSecondaryPeriod;
        case PsgSection::secondaryArpeggioNoteInOctave:
            return tagSecondaryArpeggioNoteInOctave;
        case PsgSection::secondaryArpeggioOctave:
            return tagSecondaryArpeggioOctave;
        case PsgSection::secondaryPitch:
            return tagSecondaryPitch;
        case PsgSection::sidBalancePercent:
            return tagSidBalancePercent;
        case PsgSection::sidLowShelfVolume:
            return tagSidLowShelfVolume;
        case PsgSection::sidIsActivated:
            return tagSidIsActivated;
        case PsgSection::sidArpeggio:
            return tagSidArpeggio;
        case PsgSection::sidPitch:
            return tagSidPitch;
    }
}

OptionalValue<PsgSection> PsgSectionHelper::fromString(const juce::String& string) noexcept
{
    if (string == tagSoundType) {
        return PsgSection::soundType;
    }
    if (string == tagEnvelope) {
        return PsgSection::envelope;
    }
    if (string == tagNoise) {
        return PsgSection::noise;
    }
    if (string == tagPrimaryPeriod) {
        return PsgSection::primaryPeriod;
    }
    if (string == tagPrimaryArpeggioNoteInOctave) {
        return PsgSection::primaryArpeggioNoteInOctave;
    }
    if (string == tagPrimaryArpeggioOctave) {
        return PsgSection::primaryArpeggioOctave;
    }
    if (string == tagPrimaryPitch) {
        return PsgSection::primaryPitch;
    }
    if (string == tagSecondaryPeriod) {
        return PsgSection::secondaryPeriod;
    }
    if (string == tagSecondaryArpeggioNoteInOctave) {
        return PsgSection::secondaryArpeggioNoteInOctave;
    }
    if (string == tagSecondaryArpeggioOctave) {
        return PsgSection::secondaryArpeggioOctave;
    }
    if (string == tagSecondaryPitch) {
        return PsgSection::secondaryPitch;
    }

    if (string == tagSidIsActivated) {
        return PsgSection::sidIsActivated;
    }
    if (string == tagSidBalancePercent) {
        return PsgSection::sidBalancePercent;
    }
    if (string == tagSidLowShelfVolume) {
        return PsgSection::sidLowShelfVolume;
    }
    if (string == tagSidArpeggio) {
        return PsgSection::sidArpeggio;
    }
    if (string == tagSidPitch) {
        return PsgSection::sidPitch;
    }

    jassertfalse;       // No matching String!
    return { };
}

}   // namespace arkostracker
