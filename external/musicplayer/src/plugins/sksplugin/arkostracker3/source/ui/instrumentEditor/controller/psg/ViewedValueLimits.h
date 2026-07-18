#pragma once

namespace arkostracker 
{

/** Holder of some constants about the viewed values. */
class ViewedValueLimits
{
public:
    /** Prevents instantiation. */
    ViewedValueLimits() = delete;

    static constexpr auto allEnvelopesCount = 8;
    static constexpr auto mostlyUsedEnvelopesCount = 4;
    static constexpr auto ratioCount = 8;
    static constexpr auto hardwareAllEnvelopeBarCount = allEnvelopesCount * ratioCount;
    static constexpr auto hardwareMostlyUsedEnvelopeBarCount = mostlyUsedEnvelopesCount * ratioCount;
    // All values: all the envelopes are shown.
    static constexpr auto hardwareEnvelopeBarMinimumAllValue = 0;
    static constexpr auto hardwareEnvelopeBarMaximumAllValue = hardwareAllEnvelopeBarCount - 1;
    // Most useful values: only a subset of the envelopes are shown (which is enough for 99% of the cases!).
    static constexpr auto hardwareEnvelopeBarMinimumMostUsefulValue = 0;
    static constexpr auto hardwareEnvelopeBarMaximumMostUsefulValue = hardwareMostlyUsedEnvelopeBarCount - 1;

    // Periods.
    static constexpr auto periodMaximumWhenSmall = 0x10;               // No need for more: Ben Daglish effect.
    static constexpr auto periodMaximumWhenNormal = 0x40;
    static constexpr auto periodMaximumWhenExtended = 0x600;

    // Arpeggio octave. The "all" value is defined in the PsgValues.
    static constexpr auto arpeggioOctaveBarMaximumWhenSmall = 4;
    static constexpr auto arpeggioOctaveBarMinimumWhenSmall = -arpeggioOctaveBarMaximumWhenSmall;

    // Pitch. The "all" value is defined in the PsgValues.
    static constexpr auto pitchBarMaximumWhenSmall = 0x8;
    static constexpr auto pitchBarMinimumWhenSmall = -pitchBarMaximumWhenSmall;
    static constexpr auto pitchBarMaximumWhenNormal = 0x20;
    static constexpr auto pitchBarMinimumWhenNormal = -pitchBarMaximumWhenNormal;
    static constexpr auto pitchBarMaximumWhenExtended = 0x200;
    static constexpr auto pitchBarMinimumWhenExtended = -pitchBarMaximumWhenExtended;
};

}   // namespace arkostracker
