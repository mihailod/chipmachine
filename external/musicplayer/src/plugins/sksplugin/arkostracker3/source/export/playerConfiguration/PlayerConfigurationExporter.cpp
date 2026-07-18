#include "PlayerConfigurationExporter.h"

#include "../sourceGenerator/SourceGenerator.h"
#include "PlayerConfiguration.h"

namespace arkostracker 
{

const juce::String PlayerConfigurationExporter::suffixGeneratedFilename = "_playerconfig";                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelBase = "PLY_CFG_";                                                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelConfigurationPresentForMusic = "PLY_CFG_ConfigurationIsPresent";         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelConfigurationPresentForSfx = "PLY_CFG_SFX_ConfigurationIsPresent";       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageSpeedTracks = labelBase + "UseSpeedTracks";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEventTracks = labelBase + "UseEventTracks";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageTranspositions = labelBase + "UseTranspositions";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffects = labelBase + "UseEffects";                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageRetrig = labelBase + "UseRetrig";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareSounds = labelBase + "UseHardwareSounds";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageInstrumentLoopTo = labelBase + "UseInstrumentLoopTo";              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageNoSoftNoHard = labelBase + "NoSoftNoHard";                         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardware = labelBase + "SoftToHard";                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareOnly = labelBase + "SoftOnly";                                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareOnly = labelBase + "HardOnly";                                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftware = labelBase + "HardToSoft";                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardware = labelBase + "SoftAndHard";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelSuffixNoise = "_Noise";                                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixForcedSoftwarePeriod = "_ForcedSoftwarePeriod";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixSoftwareArpeggio = "_SoftwareArpeggio";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixSoftwarePitch = "_SoftwarePitch";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelSuffixForcedHardwarePeriod = "_ForcedHardwarePeriod";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixHardwareArpeggio = "_HardwareArpeggio";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixHardwarePitch = "_HardwarePitch";                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelSuffixRetrig = "_Retrig";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageNoSoftNoHardNoise = labelManageNoSoftNoHard + labelSuffixNoise;                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageSoftwareOnlyNoise = labelManageSoftwareOnly + labelSuffixNoise;                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareOnlyForcedSoftwarePeriod = labelManageSoftwareOnly + labelSuffixForcedSoftwarePeriod;            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareOnlySoftwareArpeggio = labelManageSoftwareOnly + labelSuffixSoftwareArpeggio;                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareOnlySoftwarePitch = labelManageSoftwareOnly + labelSuffixSoftwarePitch;                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareNoise = labelManageSoftwareToHardware + labelSuffixNoise;                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareForcedSoftwarePeriod = labelManageSoftwareToHardware + labelSuffixForcedSoftwarePeriod;        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareSoftwareArpeggio = labelManageSoftwareToHardware + labelSuffixSoftwareArpeggio;                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareSoftwarePitch = labelManageSoftwareToHardware + labelSuffixSoftwarePitch;                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareHardwarePitch = labelManageSoftwareToHardware + labelSuffixHardwarePitch;                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareToHardwareRetrig = labelManageSoftwareToHardware + labelSuffixRetrig;                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageHardwareOnlyNoise = labelManageHardwareOnly + labelSuffixNoise;                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareOnlyForcedHardwarePeriod = labelManageHardwareOnly + labelSuffixForcedHardwarePeriod;            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareOnlyHardwareArpeggio = labelManageHardwareOnly + labelSuffixHardwareArpeggio;                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareOnlyHardwarePitch = labelManageHardwareOnly + labelSuffixHardwarePitch;                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareOnlyRetrig = labelManageHardwareOnly + labelSuffixRetrig;                                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareNoise = labelManageHardwareToSoftware + labelSuffixNoise;                                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareForcedHardwarePeriod = labelManageHardwareToSoftware + labelSuffixForcedHardwarePeriod;        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareHardwareArpeggio = labelManageHardwareToSoftware + labelSuffixHardwareArpeggio;                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareHardwarePitch = labelManageHardwareToSoftware + labelSuffixHardwarePitch;                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareSoftwarePitch = labelManageHardwareToSoftware + labelSuffixSoftwarePitch;                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageHardwareToSoftwareRetrig = labelManageHardwareToSoftware + labelSuffixRetrig;                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareNoise = labelManageSoftwareAndHardware + labelSuffixNoise;                                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareForcedSoftwarePeriod = labelManageSoftwareAndHardware + labelSuffixForcedSoftwarePeriod;      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareSoftwareArpeggio = labelManageSoftwareAndHardware + labelSuffixSoftwareArpeggio;              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareSoftwarePitch = labelManageSoftwareAndHardware + labelSuffixSoftwarePitch;                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareForcedHardwarePeriod = labelManageSoftwareAndHardware + labelSuffixForcedHardwarePeriod;      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareHardwareArpeggio = labelManageSoftwareAndHardware + labelSuffixHardwareArpeggio;              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareHardwarePitch = labelManageSoftwareAndHardware + labelSuffixHardwarePitch;                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSoftwareAndHardwareRetrig = labelManageSoftwareAndHardware + labelSuffixRetrig;                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageSfx = labelBase + "SFX";                                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxLoopTo = labelManageSfx + "_LoopTo";                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxNoSoftNoHard = labelManageSfx + "_NoSoftNoHard";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxNoSoftNoHardNoise = labelManageSfx + "_NoSoftNoHard_Noise";     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxSoftOnly = labelManageSfx + "_SoftOnly";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxSoftOnlyNoise = labelManageSfx + "_SoftOnly_Noise";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxHardOnly = labelManageSfx + "_HardOnly";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxHardOnlyNoise = labelManageSfx + "_HardOnly_Noise";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxHardOnlyRetrig = labelManageSfx + "_HardOnly_Retrig";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxSoftAndHard = labelManageSfx + "_SoftAndHard";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxSoftAndHardNoise = labelManageSfx + "_SoftAndHard_Noise";       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageSfxSoftAndHardRetrig = labelManageSfx + "_SoftAndHard_Retrig";     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String PlayerConfigurationExporter::labelManageBaseEffect = labelBase + "UseEffect_";                             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectLegato = labelManageBaseEffect + "Legato";                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectReset = labelManageBaseEffect + "Reset";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectForcePitchTableSpeed = labelManageBaseEffect + "ForcePitchTableSpeed";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectForceArpeggioSpeed = labelManageBaseEffect + "ForceArpeggioSpeed";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectForceInstrumentSpeed = labelManageBaseEffect + "ForceInstrumentSpeed";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectPitchGlide = labelManageBaseEffect + "PitchGlide";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectPitchUp = labelManageBaseEffect + "PitchUp";                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectPitchDown = labelManageBaseEffect + "PitchDown";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectPitchTable = labelManageBaseEffect + "PitchTable";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectArpeggio3Notes = labelManageBaseEffect + "Arpeggio3Notes";   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectArpeggio4Notes = labelManageBaseEffect + "Arpeggio4Notes";   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectArpeggioTable = labelManageBaseEffect + "ArpeggioTable";     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectSetVolume = labelManageBaseEffect + "SetVolume";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectVolumeIn = labelManageBaseEffect + "VolumeIn";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PlayerConfigurationExporter::labelManageEffectVolumeOut = labelManageBaseEffect + "VolumeOut";             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

juce::MemoryBlock PlayerConfigurationExporter::exportConfiguration(const SourceGeneratorConfiguration& sourceGeneratorConfiguration,
                                                                   const PlayerConfiguration& playerConfiguration) noexcept
{
    juce::MemoryOutputStream outputStream;
    SourceGenerator sourceGenerator(sourceGeneratorConfiguration, outputStream);

    // Adds a header.
    sourceGenerator.declareComment("Configuration that can be used by the Arkos Tracker 3 players.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareComment("It indicates what parts of code are useful to the song/sound effects, to save both memory and CPU.");
    sourceGenerator.declareComment("The players may or may not take advantage of these flags, it is up to them.");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareComment("You can either:");
    sourceGenerator.declareComment("- Ignore this file. The player will use its default build (no optimizations).");
    sourceGenerator.declareComment("- Include this to the source that also includes the player (BEFORE the player is included) (recommended solution).");
    sourceGenerator.declareComment("- Include or copy/paste this at the beginning of the player code (not recommended, the player should not be modified).");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareComment("This file was generated for a specific song. Do NOT use it for any other.");
    sourceGenerator.declareComment("Do NOT try to modify these flags, this can lead to a crash!");
    sourceGenerator.addEmptyLine();
    sourceGenerator.declareComment("If you use one player but several songs, don't worry, these declarations will stack up.");
    sourceGenerator.declareComment("Just make sure to include them, in any order, BEFORE the player.");
    sourceGenerator.addEmptyLine();

    // Converts the flags to labels.
    const auto flags = playerConfiguration.getFlags();
    jassert(!flags.empty());        // There should at least the music/sfx flag!
    for (const auto& flag : flags) {
        encodeFlag(flag, sourceGenerator);
    }

    sourceGenerator.declareEndOfFile();

    return outputStream.getMemoryBlock();
}

void PlayerConfigurationExporter::encodeFlag(PlayerConfigurationFlag flag, SourceGenerator& sourceGenerator)
{
    // Converts the flag to a label.
    static const std::unordered_map<PlayerConfigurationFlag, juce::String> flagToLabel = {

            { PlayerConfigurationFlag::music,                                   labelConfigurationPresentForMusic },
            { PlayerConfigurationFlag::sfx,                                     labelConfigurationPresentForSfx },

            { PlayerConfigurationFlag::retrig,                                  labelManageRetrig },
            { PlayerConfigurationFlag::instrumentLoopTo,                        labelManageInstrumentLoopTo },

            { PlayerConfigurationFlag::effects,                                 labelManageEffects },

            { PlayerConfigurationFlag::speedTracks,                             labelManageSpeedTracks },
            { PlayerConfigurationFlag::eventTracks,                             labelManageEventTracks },

            { PlayerConfigurationFlag::transpositions,                          labelManageTranspositions },

            { PlayerConfigurationFlag::hardwareSounds,                          labelManageHardwareSounds },

            { PlayerConfigurationFlag::noSoftNoHard,                            labelManageNoSoftNoHard },
            { PlayerConfigurationFlag::noSoftNoHardNoise,                       labelManageNoSoftNoHardNoise },

            { PlayerConfigurationFlag::softwareOnly,                            labelManageSoftwareOnly },
            { PlayerConfigurationFlag::softwareOnlyNoise,                       labelManageSoftwareOnlyNoise },
            { PlayerConfigurationFlag::softwareOnlyForcedSoftwarePeriod,        labelManageSoftwareOnlyForcedSoftwarePeriod },
            { PlayerConfigurationFlag::softwareOnlySoftwareArpeggio,            labelManageSoftwareOnlySoftwareArpeggio },
            { PlayerConfigurationFlag::softwareOnlySoftwarePitch,               labelManageSoftwareOnlySoftwarePitch },

            { PlayerConfigurationFlag::hardwareOnly,                            labelManageHardwareOnly },
            { PlayerConfigurationFlag::hardwareOnlyNoise,                       labelManageHardwareOnlyNoise },
            { PlayerConfigurationFlag::hardwareOnlyForcedHardwarePeriod,        labelManageHardwareOnlyForcedHardwarePeriod },
            { PlayerConfigurationFlag::hardwareOnlyHardwareArpeggio,            labelManageHardwareOnlyHardwareArpeggio },
            { PlayerConfigurationFlag::hardwareOnlyHardwarePitch,               labelManageHardwareOnlyHardwarePitch },
            { PlayerConfigurationFlag::hardwareOnlyRetrig,                      labelManageHardwareOnlyRetrig },

            { PlayerConfigurationFlag::softwareToHardware,                      labelManageSoftwareToHardware },
            { PlayerConfigurationFlag::softwareToHardwareNoise,                 labelManageSoftwareToHardwareNoise },
            { PlayerConfigurationFlag::softwareToHardwareForcedSoftwarePeriod,  labelManageSoftwareToHardwareForcedSoftwarePeriod },
            { PlayerConfigurationFlag::softwareToHardwareSoftwareArpeggio,      labelManageSoftwareToHardwareSoftwareArpeggio },
            { PlayerConfigurationFlag::softwareToHardwareSoftwarePitch,         labelManageSoftwareToHardwareSoftwarePitch },
            { PlayerConfigurationFlag::softwareToHardwareHardwarePitch,         labelManageSoftwareToHardwareHardwarePitch },
            { PlayerConfigurationFlag::softwareToHardwareRetrig,                labelManageSoftwareToHardwareRetrig },

            { PlayerConfigurationFlag::hardwareToSoftware,                      labelManageHardwareToSoftware },
            { PlayerConfigurationFlag::hardwareToSoftwareNoise,                 labelManageHardwareToSoftwareNoise },
            { PlayerConfigurationFlag::hardwareToSoftwareForcedHardwarePeriod,  labelManageHardwareToSoftwareForcedHardwarePeriod },
            { PlayerConfigurationFlag::hardwareToSoftwareHardwareArpeggio,      labelManageHardwareToSoftwareHardwareArpeggio },
            { PlayerConfigurationFlag::hardwareToSoftwareHardwarePitch,         labelManageHardwareToSoftwareHardwarePitch },
            { PlayerConfigurationFlag::hardwareToSoftwareSoftwarePitch,         labelManageHardwareToSoftwareSoftwarePitch },
            { PlayerConfigurationFlag::hardwareToSoftwareRetrig,                labelManageHardwareToSoftwareRetrig },

            { PlayerConfigurationFlag::softwareAndHardware,                     labelManageSoftwareAndHardware },
            { PlayerConfigurationFlag::softwareAndHardwareNoise,                labelManageSoftwareAndHardwareNoise },
            { PlayerConfigurationFlag::softwareAndHardwareForcedSoftwarePeriod, labelManageSoftwareAndHardwareForcedSoftwarePeriod },
            { PlayerConfigurationFlag::softwareAndHardwareSoftwareArpeggio,     labelManageSoftwareAndHardwareSoftwareArpeggio },
            { PlayerConfigurationFlag::softwareAndHardwareSoftwarePitch,        labelManageSoftwareAndHardwareSoftwarePitch },
            { PlayerConfigurationFlag::softwareAndHardwareForcedHardwarePeriod, labelManageSoftwareAndHardwareForcedHardwarePeriod },
            { PlayerConfigurationFlag::softwareAndHardwareHardwareArpeggio,     labelManageSoftwareAndHardwareHardwareArpeggio },
            { PlayerConfigurationFlag::softwareAndHardwareHardwarePitch,        labelManageSoftwareAndHardwareHardwarePitch },
            { PlayerConfigurationFlag::softwareAndHardwareRetrig,               labelManageSoftwareAndHardwareRetrig },

            // Effects.
            // --------------------------------------
            { PlayerConfigurationFlag::effectSetVolume,                         labelManageEffectSetVolume },
            { PlayerConfigurationFlag::effectVolumeIn,                          labelManageEffectVolumeIn },
            { PlayerConfigurationFlag::effectVolumeOut,                         labelManageEffectVolumeOut },

            { PlayerConfigurationFlag::effectArpeggioTable,                     labelManageEffectArpeggioTable },
            { PlayerConfigurationFlag::effectArpeggio3Notes,                    labelManageEffectArpeggio3Notes },
            { PlayerConfigurationFlag::effectArpeggio4Notes,                    labelManageEffectArpeggio4Notes },

            { PlayerConfigurationFlag::effectPitchTable,                        labelManageEffectPitchTable },
            { PlayerConfigurationFlag::effectPitchUp,                           labelManageEffectPitchUp },
            { PlayerConfigurationFlag::effectPitchDown,                         labelManageEffectPitchDown },
            { PlayerConfigurationFlag::effectPitchGlide,                        labelManageEffectPitchGlide },

            { PlayerConfigurationFlag::effectLegato,                            labelManageEffectLegato },
            { PlayerConfigurationFlag::effectReset,                             labelManageEffectReset },
            { PlayerConfigurationFlag::effectForceInstrumentSpeed,              labelManageEffectForceInstrumentSpeed },
            { PlayerConfigurationFlag::effectForceArpeggioSpeed,                labelManageEffectForceArpeggioSpeed },
            { PlayerConfigurationFlag::effectForcePitchTableSpeed,              labelManageEffectForcePitchTableSpeed },

            // Sound effects.
            // --------------------------------------
            { PlayerConfigurationFlag::sfxLoopTo,                               labelManageSfxLoopTo },
            { PlayerConfigurationFlag::sfxNoSoftNoHard,                         labelManageSfxNoSoftNoHard },
            { PlayerConfigurationFlag::sfxNoSoftNoHardNoise,                    labelManageSfxNoSoftNoHardNoise },
            { PlayerConfigurationFlag::sfxSoftOnly,                             labelManageSfxSoftOnly },
            { PlayerConfigurationFlag::sfxSoftOnlyNoise,                        labelManageSfxSoftOnlyNoise },
            { PlayerConfigurationFlag::sfxHardOnly,                             labelManageSfxHardOnly },
            { PlayerConfigurationFlag::sfxHardOnlyNoise,                        labelManageSfxHardOnlyNoise },
            { PlayerConfigurationFlag::sfxHardOnlyRetrig,                       labelManageSfxHardOnlyRetrig },
            { PlayerConfigurationFlag::sfxSoftAndHard,                          labelManageSfxSoftAndHard },
            { PlayerConfigurationFlag::sfxSoftAndHardNoise,                     labelManageSfxSoftAndHardNoise },
            { PlayerConfigurationFlag::sfxSoftAndHardRetrig,                    labelManageSfxSoftAndHardRetrig },

    };

    const auto iterator = flagToLabel.find(flag);
    if (iterator == flagToLabel.cend()) {
        jassertfalse;
        return;     // Shouldn't happen.
    }

    sourceGenerator.declareConstant(iterator->second, "1");
}

}   // namespace arkostracker
