#pragma once

#include <cstdint>

namespace arkostracker 
{

/** The errors that can be detected when browsing effects. */
enum class EffectError : std::uint8_t
{
    tooManyPitchEffects,
    tooManyPitchTableEffects,
    tooManyVolumeEffects,
    tooManyVolumeSlideEffects,
    tooManyArpeggioEffects,
    tooManyResetEffects,
    tooManyLegatoEffects,                   // Only for generators.
    tooManyForceInstrumentSpeedEffects,
    tooManyForceArpeggioSpeedEffects,
    tooManyForcePitchTableSpeedEffects,

    removeEffectBeforeReset,

    /** After a reset, Pitch/Arp table/inline at 0, Speeds at 0, are useless. */
    uselessEffectStopAfterReset,

    volumeHidingVolumeSlide,
    /** Set Volume after a reset must be combined to it. */
    volumeMustBeCombinedWithReset,

    arpeggioTableOrDirectHidingForceSpeed,
    pitchTableHidingForceSpeed,

    pitchGlideAfterResetIsMeaningless,
    pitchGlideWithInstrumentIsForbidden,
    pitchGlide0WithNoteIsUseless,
    pitchAndGlide0WithNewInstrumentIsUseless,

    // =====================
    count,
    countForEditor = count - 1          // Removed the "too many legato".
};

}   // namespace arkostracker

