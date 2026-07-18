#include "PsgFrequency.h"

namespace arkostracker 
{

const int PsgFrequency::psgFrequencyCPC = 1000000;
const int PsgFrequency::psgFrequencyPentagon128K = 1750000;
const int PsgFrequency::psgFrequencySpectrum = 1773400;
const int PsgFrequency::psgFrequencyMsxAndSvi = 1789773;
const int PsgFrequency::psgFrequencySvi3x8 = psgFrequencyMsxAndSvi;
const int PsgFrequency::psgFrequencyAtariST = 2000000;
const int PsgFrequency::psgFrequencyAtariXEXL = 1773400;
const int PsgFrequency::psgFrequencySharpMz700 = 894886;
const int PsgFrequency::psgFrequencyApple2 = 1000000;
const int PsgFrequency::psgFrequencyOric = 1000000;
const int PsgFrequency::psgFrequencyVectrex = 1500000;
const int PsgFrequency::psgFrequencySpecNext = psgFrequencySpectrum;

const float PsgFrequency::minimumReplayFrequencyHz = 5.0F;
const float PsgFrequency::maximumReplayFrequencyHz = 5000.0F;
const float PsgFrequency::defaultReplayFrequencyHz = 50.0F;

const float PsgFrequency::defaultReferenceFrequencyHz = 440.0F;

const int PsgFrequency::defaultSamplePlayerFrequencyHz = 8000;  // Because a player usually is every 2 scanlines.
const int PsgFrequency::defaultModSamplePlayerFrequencyHz = 8373;   // Matched most MOD samples, and Paula "C" note.
const int PsgFrequency::defaultSampleFrequencyHz = 11025;

const int PsgFrequency::minimumPsgFrequency = 5000;
const int PsgFrequency::maximumPsgFrequency = 8000000;

const float PsgFrequency::minimumReferenceFrequency = 100.0F;
const float PsgFrequency::maximumReferenceFrequency = 1000.0F;

const int PsgFrequency::minimumSamplePlayerFrequency = 1000;
const int PsgFrequency::maximumSamplePlayerFrequency = 44100;

const int PsgFrequency::minimumSampleFrequency = 1000;
const int PsgFrequency::maximumSampleFrequency = 44100;

juce::String PsgFrequency::frequencyToPlatform(const int psgFrequency) noexcept
{
    switch (psgFrequency) {
        case psgFrequencyCPC: return "Amstrad CPC";
        case psgFrequencyPentagon128K: return "Pentagon 128K";
        case psgFrequencySpectrum: return "Spectrum";
        case psgFrequencyMsxAndSvi: return "MSX / SVI 3x8";
        case psgFrequencyAtariST: return "Atari ST";
        case psgFrequencySharpMz700: return "Sharp MZ 700";
        case psgFrequencyVectrex: return "Vectrex";
        default: return "Unknown";
    }
}

}   // namespace arkostracker
