#pragma once

#include <juce_core/juce_core.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** The various type of "section". Useful for handling auto-spread method. */
enum class PsgSection : uint8_t
{
    soundType,
    envelope,
    noise,

    primaryPeriod,
    primaryArpeggioNoteInOctave,
    primaryArpeggioOctave,
    primaryPitch,

    secondaryPeriod,
    secondaryArpeggioNoteInOctave,
    secondaryArpeggioOctave,
    secondaryPitch,

    sidBalancePercent,
    sidLowShelfVolume,
    sidIsActivated,
    //sidRatio,           // NOTE: this is actually not used, as it is encoded with IsActivated. May be split one day?
    sidArpeggio,
    sidPitch,

    count
};

/** Helper to serialize/deserialize the PsgSection. */
class PsgSectionHelper
{
public:
    /** Prevents instantiation. */
    PsgSectionHelper() = delete;

    static const juce::String tagSoundType;
    static const juce::String tagEnvelope;
    static const juce::String tagNoise;
    static const juce::String tagPrimaryPeriod;
    static const juce::String tagPrimaryArpeggioNoteInOctave;
    static const juce::String tagPrimaryArpeggioOctave;
    static const juce::String tagPrimaryPitch;
    static const juce::String tagSecondaryPeriod;
    static const juce::String tagSecondaryArpeggioNoteInOctave;
    static const juce::String tagSecondaryArpeggioOctave;
    static const juce::String tagSecondaryPitch;
    static const juce::String tagSidBalancePercent;
    static const juce::String tagSidLowShelfVolume;
    static const juce::String tagSidIsActivated;
    static const juce::String tagSidArpeggio;
    static const juce::String tagSidPitch;

    /**
     * @return a string from the given PSG. The String is made for serialization only.
     * @param psgSection the PSG Section.
     */
    static juce::String toString(PsgSection psgSection) noexcept;

    /**
     * @return the PSG Section from the given String.
     * @param string the PsgSection, or empty if couldn't be parsed.
     */
    static OptionalValue<PsgSection> fromString(const juce::String& string) noexcept;
};

}   // namespace arkostracker

