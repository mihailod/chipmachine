#pragma once

namespace arkostracker 
{

/** The encoded effects. */
enum class EncodedEffect : unsigned char
{
    resetFullVolume,
    reset,
    volume,
    arpeggioTable,
    arpeggioTableStop,
    pitchTable,
    pitchTableStop,
    volumeSlide,
    volumeSlideStop,
    pitchUp,
    pitchDown,
    pitchStop,                  // Also used for Glide Stop.
    glideWithNote,
    glideSpeed,
    legato,
    forceInstrumentSpeed,
    forceArpeggioSpeed,
    forcePitchTableSpeed,
};

}   // namespace arkostracker

