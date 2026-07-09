#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Frequencies of various PSGs, in Hz. */
class PsgFrequency
{
public:
    /** Prevents instantiation. */
    PsgFrequency() = delete;

    static const int psgFrequencyCPC;                   // The PSG frequency for the CPC.
    static const int psgFrequencyPentagon128K;          // The PSG frequency for the Pentagon128K.
    static const int psgFrequencySpectrum;              // The PSG frequency for the Spectrum.
    static const int psgFrequencyMsxAndSvi;             // The PSG frequency for the MSX.
    static const int psgFrequencySvi3x8;                // The PSG frequency for the Spectravideo SVI 318/328.
    static const int psgFrequencyAtariST;               // The PSG frequency for the Atari ST.
    static const int psgFrequencySharpMz700;            // The PSG frequency for the Sharp MZ700.
    static const int psgFrequencyAtariXEXL;             // The PSG frequency for the Atari XE/XL.
    static const int psgFrequencyApple2;                // The PSG frequency for the Apple 2.
    static const int psgFrequencyOric;                  // The PSG frequency for the Oric.
    static const int psgFrequencyVectrex;               // The PSG frequency for the Vectrex.
    static const int psgFrequencySpecNext;              // The PSG frequency for the Spectrum Next.

    static const float minimumReplayFrequencyHz;
    static const float maximumReplayFrequencyHz;
    static const float defaultReplayFrequencyHz;        // The default player replay, in Hz.

    static const float defaultReferenceFrequencyHz;     // The default reference frequency.

    static const int defaultSamplePlayerFrequencyHz;    // The default sample player frequency.
    static const int defaultModSamplePlayerFrequencyHz;
    static const int defaultSampleFrequencyHz;

    static const int minimumPsgFrequency;
    static const int maximumPsgFrequency;

    static const float minimumReferenceFrequency;
    static const float maximumReferenceFrequency;

    static const int minimumSamplePlayerFrequency;
    static const int maximumSamplePlayerFrequency;

    static const int minimumSampleFrequency;
    static const int maximumSampleFrequency;

    /**
     * @return a displayable String of a platform, according to the PSG frequency ("Amstrad CPC" for example), or "Unknown".
     * @param psgFrequency the PSG frequency.
     */
    static juce::String frequencyToPlatform(int psgFrequency) noexcept;
};

}   // namespace arkostracker
