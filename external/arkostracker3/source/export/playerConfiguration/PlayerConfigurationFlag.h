#pragma once

namespace arkostracker 
{

/** Flags indicating what flags are used. */
enum class PlayerConfigurationFlag
{
    music,
    sfx,

    instrumentLoopTo,   // True if at least one Instrument uses the "loop". The first/empty Instrument is NOT taken in account! We consider it doesn't loop.
    retrig,             // True if at least one Instrument uses a Retrig. Automatically set when other Retrigs are set.

    effects,            // True when at least one effect is used.

    speedTracks,        // True as soon as one value is written.
    eventTracks,        // True as soon as one value is written.

    transpositions,     // True as soon as one non-zeo transposition is written.

    hardwareSounds,     // True as soon as one hardware sound used.

    noSoftNoHard,
    noSoftNoHardNoise,

    softwareOnly,
    softwareOnlyNoise,
    softwareOnlyForcedSoftwarePeriod,
    softwareOnlySoftwarePitch,
    softwareOnlySoftwareArpeggio,

    hardwareOnly,
    hardwareOnlyNoise,
    hardwareOnlyForcedHardwarePeriod,
    hardwareOnlyHardwareArpeggio,
    hardwareOnlyHardwarePitch,
    hardwareOnlyRetrig,

    softwareToHardware,
    softwareToHardwareNoise,
    softwareToHardwareForcedSoftwarePeriod,
    softwareToHardwareSoftwareArpeggio,
    softwareToHardwareSoftwarePitch,
    softwareToHardwareHardwarePitch,
    softwareToHardwareRetrig,

    hardwareToSoftware,
    hardwareToSoftwareNoise,
    hardwareToSoftwareForcedHardwarePeriod,
    hardwareToSoftwareHardwareArpeggio,
    hardwareToSoftwareHardwarePitch,
    hardwareToSoftwareSoftwarePitch,
    hardwareToSoftwareRetrig,

    softwareAndHardware,
    softwareAndHardwareNoise,
    softwareAndHardwareForcedSoftwarePeriod,
    softwareAndHardwareSoftwareArpeggio,
    softwareAndHardwareSoftwarePitch,
    softwareAndHardwareForcedHardwarePeriod,
    softwareAndHardwareHardwareArpeggio,
    softwareAndHardwareHardwarePitch,
    softwareAndHardwareRetrig,

    // Effects.
    // --------------------------------------
    effectSetVolume,
    effectVolumeIn,
    effectVolumeOut,

    effectArpeggio3Notes,
    effectArpeggio4Notes,
    effectArpeggioTable,

    effectPitchTable,
    effectPitchUp,
    effectPitchDown,
    effectPitchGlide,

    effectLegato,
    effectReset,
    effectForceInstrumentSpeed,
    effectForceArpeggioSpeed,
    effectForcePitchTableSpeed,

    // Sound effects.
    // --------------------------------------
    sfxLoopTo,
    sfxNoSoftNoHard,
    sfxNoSoftNoHardNoise,
    sfxSoftOnly,
    sfxSoftOnlyNoise,
    sfxHardOnly,
    sfxHardOnlyNoise,
    sfxHardOnlyRetrig,
    sfxSoftAndHard,
    sfxSoftAndHardNoise,
    sfxSoftAndHardRetrig,
};

}   // namespace arkostracker

