#pragma once

#include <juce_core/juce_core.h>

#include "PlayerConfigurationFlag.h"

namespace arkostracker 
{

class PlayerConfiguration;
class SourceGeneratorConfiguration;
class SourceGenerator;

/**
 * Class to generate, as a source file, the configuration of the player according to what an exporter might declare.
 * Contrary to AT2, the Song itself won't be browsed, producing statistics. This is now done by the exporters directly, allowing
 * more accurate declaration, as well as player-specific ones.
 */
class PlayerConfigurationExporter
{
public:
    static const juce::String suffixGeneratedFilename;            // Suffix added to the file name of the configuration.

    /** Prevents instantiation. */
    PlayerConfigurationExporter() = delete;

    /**
     * Exports the configuration to source.
     * @param sourceGeneratorConfiguration the source configuration.
     * @param playerConfiguration the player configuration.
     * @return the source code of the configuration.
     */
    static juce::MemoryBlock exportConfiguration(const SourceGeneratorConfiguration& sourceGeneratorConfiguration, const PlayerConfiguration& playerConfiguration) noexcept;

private:
    static const juce::String labelBase;
    static const juce::String labelConfigurationPresentForMusic;
    static const juce::String labelConfigurationPresentForSfx;

    static const juce::String labelManageSpeedTracks;
    static const juce::String labelManageEventTracks;
    static const juce::String labelManageTranspositions;

    static const juce::String labelManageEffects;

    static const juce::String labelManageRetrig;
    static const juce::String labelManageHardwareSounds;

    static const juce::String labelManageInstrumentLoopTo;

    static const juce::String labelSuffixNoise;
    static const juce::String labelSuffixForcedSoftwarePeriod;
    static const juce::String labelSuffixSoftwareArpeggio;
    static const juce::String labelSuffixSoftwarePitch;
    static const juce::String labelSuffixForcedHardwarePeriod;
    static const juce::String labelSuffixHardwareArpeggio;
    static const juce::String labelSuffixHardwarePitch;
    static const juce::String labelSuffixRetrig;

    static const juce::String labelManageNoSoftNoHard;
    static const juce::String labelManageNoSoftNoHardNoise;

    static const juce::String labelManageSoftwareOnly;
    static const juce::String labelManageSoftwareOnlyNoise;
    static const juce::String labelManageSoftwareOnlyForcedSoftwarePeriod;
    static const juce::String labelManageSoftwareOnlySoftwareArpeggio;
    static const juce::String labelManageSoftwareOnlySoftwarePitch;

    static const juce::String labelManageSoftwareToHardware;
    static const juce::String labelManageSoftwareToHardwareNoise;
    static const juce::String labelManageSoftwareToHardwareForcedSoftwarePeriod;
    static const juce::String labelManageSoftwareToHardwareSoftwareArpeggio;
    static const juce::String labelManageSoftwareToHardwareSoftwarePitch;
    static const juce::String labelManageSoftwareToHardwareHardwarePitch;
    static const juce::String labelManageSoftwareToHardwareRetrig;

    static const juce::String labelManageHardwareOnly;
    static const juce::String labelManageHardwareOnlyNoise;
    static const juce::String labelManageHardwareOnlyForcedHardwarePeriod;
    static const juce::String labelManageHardwareOnlyHardwareArpeggio;
    static const juce::String labelManageHardwareOnlyHardwarePitch;
    static const juce::String labelManageHardwareOnlyRetrig;

    static const juce::String labelManageHardwareToSoftware;
    static const juce::String labelManageHardwareToSoftwareNoise;
    static const juce::String labelManageHardwareToSoftwareForcedHardwarePeriod;
    static const juce::String labelManageHardwareToSoftwareHardwareArpeggio;
    static const juce::String labelManageHardwareToSoftwareHardwarePitch;
    static const juce::String labelManageHardwareToSoftwareSoftwarePitch;
    static const juce::String labelManageHardwareToSoftwareRetrig;

    static const juce::String labelManageSoftwareAndHardware;
    static const juce::String labelManageSoftwareAndHardwareNoise;
    static const juce::String labelManageSoftwareAndHardwareForcedSoftwarePeriod;
    static const juce::String labelManageSoftwareAndHardwareSoftwareArpeggio;
    static const juce::String labelManageSoftwareAndHardwareSoftwarePitch;
    static const juce::String labelManageSoftwareAndHardwareForcedHardwarePeriod;
    static const juce::String labelManageSoftwareAndHardwareHardwareArpeggio;
    static const juce::String labelManageSoftwareAndHardwareHardwarePitch;
    static const juce::String labelManageSoftwareAndHardwareRetrig;

    static const juce::String labelManageSfx;

    static const juce::String labelManageSfxLoopTo;
    static const juce::String labelManageSfxNoSoftNoHard;
    static const juce::String labelManageSfxNoSoftNoHardNoise;
    static const juce::String labelManageSfxSoftOnly;
    static const juce::String labelManageSfxSoftOnlyNoise;
    static const juce::String labelManageSfxHardOnly;
    static const juce::String labelManageSfxHardOnlyNoise;
    static const juce::String labelManageSfxHardOnlyRetrig;
    static const juce::String labelManageSfxSoftAndHard;
    static const juce::String labelManageSfxSoftAndHardNoise;
    static const juce::String labelManageSfxSoftAndHardRetrig;

    static const juce::String labelManageBaseEffect;
    static const juce::String labelManageEffectLegato;
    static const juce::String labelManageEffectReset;
    static const juce::String labelManageEffectForcePitchTableSpeed;
    static const juce::String labelManageEffectForceArpeggioSpeed;
    static const juce::String labelManageEffectForceInstrumentSpeed;
    static const juce::String labelManageEffectPitchGlide;
    static const juce::String labelManageEffectPitchUp;
    static const juce::String labelManageEffectPitchDown;
    static const juce::String labelManageEffectPitchTable;
    static const juce::String labelManageEffectArpeggio3Notes;
    static const juce::String labelManageEffectArpeggio4Notes;
    static const juce::String labelManageEffectArpeggioTable;
    static const juce::String labelManageEffectSetVolume;
    static const juce::String labelManageEffectVolumeIn;
    static const juce::String labelManageEffectVolumeOut;

    /**
     * Encodes a flag to the source generator.
     * @param flag the flag.
     * @param sourceGenerator the source generator.
     */
    static void encodeFlag(PlayerConfigurationFlag flag, SourceGenerator& sourceGenerator);
};

}   // namespace arkostracker

