#include "PlayerConfiguration.h"

namespace arkostracker 
{

PlayerConfiguration::PlayerConfiguration(const bool isMusic) noexcept :
        flags()
{
    addFlag(isMusic ? PlayerConfigurationFlag::music : PlayerConfigurationFlag::sfx);
}

void PlayerConfiguration::addFlag(const bool mustAddFlag, const PlayerConfigurationFlag flag) noexcept
{
    if (mustAddFlag) {
        addFlag(flag);
    }
}

void PlayerConfiguration::addFlag(const PlayerConfigurationFlag flag) noexcept
{
    flags.insert(flag);

    // Special cases, the flag may trigger other flags.
    switch (flag) {
        // If a retrig is set, sets the generic retrig.
        case PlayerConfigurationFlag::hardwareOnlyRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnlyRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHardRetrig:
            flags.insert(PlayerConfigurationFlag::retrig);
            break;
        case PlayerConfigurationFlag::effectSetVolume: [[fallthrough]];
        case PlayerConfigurationFlag::effectVolumeIn: [[fallthrough]];
        case PlayerConfigurationFlag::effectVolumeOut: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggio3Notes: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggio4Notes: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggioTable: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchTable: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchUp: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchDown: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchGlide: [[fallthrough]];
        case PlayerConfigurationFlag::effectLegato: [[fallthrough]];
        case PlayerConfigurationFlag::effectReset: [[fallthrough]];
        case PlayerConfigurationFlag::effectForceInstrumentSpeed: [[fallthrough]];
        case PlayerConfigurationFlag::effectForceArpeggioSpeed: [[fallthrough]];
        case PlayerConfigurationFlag::effectForcePitchTableSpeed:
            flags.insert(PlayerConfigurationFlag::effects);
            break;
        case PlayerConfigurationFlag::music: [[fallthrough]];
        case PlayerConfigurationFlag::sfx: [[fallthrough]];
        case PlayerConfigurationFlag::instrumentLoopTo: [[fallthrough]];
        case PlayerConfigurationFlag::retrig: [[fallthrough]];
        case PlayerConfigurationFlag::effects: [[fallthrough]];
        case PlayerConfigurationFlag::speedTracks: [[fallthrough]];
        case PlayerConfigurationFlag::eventTracks: [[fallthrough]];
        case PlayerConfigurationFlag::transpositions: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareSounds: [[fallthrough]];
        case PlayerConfigurationFlag::noSoftNoHard: [[fallthrough]];
        case PlayerConfigurationFlag::noSoftNoHardNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnly: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlyForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlySoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlySoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnly: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardware: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareSoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftware: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardware: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareSoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::sfxLoopTo: [[fallthrough]];
        case PlayerConfigurationFlag::sfxNoSoftNoHard: [[fallthrough]];
        case PlayerConfigurationFlag::sfxNoSoftNoHardNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftOnly: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnly: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHard: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHardNoise: [[fallthrough]];
        default:
            break;
    }

    // Second pass for the hardware sounds. Only the "base" ones are tested, should be enough (and less error-prone).
    switch (flag) {
        case PlayerConfigurationFlag::hardwareToSoftware: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnly: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardware: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardware:
            flags.insert(PlayerConfigurationFlag::hardwareSounds);
            break;
        case PlayerConfigurationFlag::music: [[fallthrough]];
        case PlayerConfigurationFlag::sfx: [[fallthrough]];
        case PlayerConfigurationFlag::instrumentLoopTo: [[fallthrough]];
        case PlayerConfigurationFlag::retrig: [[fallthrough]];
        case PlayerConfigurationFlag::effects: [[fallthrough]];
        case PlayerConfigurationFlag::speedTracks: [[fallthrough]];
        case PlayerConfigurationFlag::eventTracks: [[fallthrough]];
        case PlayerConfigurationFlag::transpositions: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareSounds: [[fallthrough]];
        case PlayerConfigurationFlag::noSoftNoHard: [[fallthrough]];
        case PlayerConfigurationFlag::noSoftNoHardNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnly: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlyForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlySoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareOnlySoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareOnlyRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareSoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareToHardwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::hardwareToSoftwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareNoise: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareForcedSoftwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareSoftwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareSoftwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareForcedHardwarePeriod: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareHardwareArpeggio: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareHardwarePitch: [[fallthrough]];
        case PlayerConfigurationFlag::softwareAndHardwareRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::effectSetVolume: [[fallthrough]];
        case PlayerConfigurationFlag::effectVolumeIn: [[fallthrough]];
        case PlayerConfigurationFlag::effectVolumeOut: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggio3Notes: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggio4Notes: [[fallthrough]];
        case PlayerConfigurationFlag::effectArpeggioTable: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchTable: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchUp: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchDown: [[fallthrough]];
        case PlayerConfigurationFlag::effectPitchGlide: [[fallthrough]];
        case PlayerConfigurationFlag::effectLegato: [[fallthrough]];
        case PlayerConfigurationFlag::effectReset: [[fallthrough]];
        case PlayerConfigurationFlag::effectForceInstrumentSpeed: [[fallthrough]];
        case PlayerConfigurationFlag::effectForceArpeggioSpeed: [[fallthrough]];
        case PlayerConfigurationFlag::effectForcePitchTableSpeed: [[fallthrough]];
        case PlayerConfigurationFlag::sfxLoopTo: [[fallthrough]];
        case PlayerConfigurationFlag::sfxNoSoftNoHard: [[fallthrough]];
        case PlayerConfigurationFlag::sfxNoSoftNoHardNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftOnly: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnly: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnlyNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxHardOnlyRetrig: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHard: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHardNoise: [[fallthrough]];
        case PlayerConfigurationFlag::sfxSoftAndHardRetrig: [[fallthrough]];
        default:
            break;
    }
}

std::set<PlayerConfigurationFlag> PlayerConfiguration::getFlags() const noexcept
{
    return flags;
}

}   // namespace arkostracker
