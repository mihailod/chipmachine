#pragma once

#include <utility>
#include <vector>

namespace arkostracker 
{

/** Holds constants about the PSG, such as minimum and maximum values of each register. */
class PsgValues
{
public:
    /** Prevents instantiation. */
    PsgValues() = delete;

    /**
     * Calculates how many PSGs are necessary to hold the given channel count, knowing that each PSG can hold three channels.
     * @param channelCount the channel count (>=0).
     * @return the PSG count (>0).
     */
    static int getPsgCount(int channelCount) noexcept;

    /**
     * @return how many channels there are for the given PSG count.
     * @param psgCount the PSG count (>0).
     */
    static int getChannelCount(int psgCount) noexcept;

    /**
     * Returns the channel index, according to the given index (0-2) and the PSG index.
     * @param channelIndex the channel index (0-2).
     * @param psgIndex the PSG index.
     * @return the channel index for the given PSG.
     */
    static int getChannelIndex(int channelIndex, int psgIndex) noexcept;

    /**
     * @return a channel index (0-2), according to the given "linear" channel index (0-x).
     * @param linearChannelIndex the channel index (0-x).
     */
    static int getPsgChannelIndex(int linearChannelIndex) noexcept;

    /**
     * @return the PSG index from the given channel index.
     * @param channelIndex the channel index (may be >2).
     */
    static int getPsgIndex(int channelIndex) noexcept;

    /**
     * @return the first and last channel index for the given PSG.
     * @param psgIndex the PSG index. For example, with 0, returns 0 and 2.
     */
    static std::pair<int, int> getFirstAndLastChannels(int psgIndex) noexcept;

    /**
     * @return the channels related to the given PSG. For example, returns 3, 4, 5 for the PSG 1.
     * @param psgIndex the PSG index.
     */
    static std::vector<int> getChannelsFromPsg(int psgIndex) noexcept;

    /**
     * @return the PSG index and channel index in the PSG, from the channel index in the Song (may be >2!).
     * @param channelIndexInSong the channel index in the song (0-8 for 3 PSGs for example).
     */
    static std::pair<int, int> getPsgAndChannelIndexes(int channelIndexInSong) noexcept;

    /**
     * Converts a hardware envelope number to AT3, as AT3 is more selective.
     * @param envelope the envelope from 0 to 15. Superior bits are removed.
     * @return the envelope from 8-15.
     */
    static int convertEnvelopeCurveToAt(int envelope) noexcept;

    static constexpr auto registerSoftwareFrequencyChannel1LSB = 0;
    static constexpr auto registerSoftwareFrequencyChannel1MSB = 1;
    static constexpr auto registerSoftwareFrequencyChannel2LSB = 2;
    static constexpr auto registerSoftwareFrequencyChannel2MSB = 3;
    static constexpr auto registerSoftwareFrequencyChannel3LSB = 4;
    static constexpr auto registerSoftwareFrequencyChannel3MSB = 5;
    static constexpr auto registerNoise = 6;
    static constexpr auto registerMixer = 7;
    static constexpr auto registerVolumeChannel1 = 8;
    static constexpr auto registerVolumeChannel2 = 9;
    static constexpr auto registerVolumeChannel3 = 10;
    static constexpr auto registerHardwareFrequencyLSB = 11;
    static constexpr auto registerHardwareFrequencyMSB = 12;
    static constexpr auto registerHardwareEnvelope = 13;

    static constexpr auto channelCountPerPsg = 3;                          // How many channels in a PSG.

    static constexpr auto digidrumNote = 12 * 6;

    static constexpr auto minimumRatio = 0;
    static constexpr auto maximumRatio = 7;
    static constexpr auto defaultRatio = 4;
    static constexpr auto minimumPeriod = 0;                               // Minimum value of the software period (R0-R5).
    static constexpr auto maximumPeriod = 0xffff;                          // Maximum value, taking the hardware one account because larger.
    static constexpr auto maximumSoftwarePeriod = 0xfff;                   // Maximum value of the software period (R0-R5).
    static constexpr auto maximumHardwarePeriod = 0xffff;                  // Maximum value of the hardware period (R11-R12).
    static constexpr auto minimumNoise = 0;                                // Minimum value of the noise (R6).
    static constexpr auto maximumNoise = 0x1f;                             // Maximum value of the noise (R6).
    static constexpr auto minimumMixer = 0;                                // Minimum value of the mixer (R7).
    static constexpr auto maximumMixer = 0b111111;                         // Maximum value of the mixer (R7).
    static constexpr auto minimumVolume = 0;                               // Minimum value of the volumes (R8-R10).
    static constexpr auto maximumVolumeNoHard = 15;                        // Maximum value of the volumes (R8-R10), without the hardware volume.
    static constexpr auto maximumVolumeIncludeHardwareVolume = 16;         // Maximum value of the volumes (R8-R10).
    static const int minimumHardwareEnvelope;                         // Minimum value of the envelope (R13) (0-7 should be set to 8-15).
    static constexpr auto maximumHardwareEnvelope = 0xf;                   // Maximum value of the envelope (R13).
    static constexpr auto defaultHardwareEnvelope = 8;                     // Default value of the envelope (R13).

    // Actually linked to a PSG Cell, not PSG constants really.
    static constexpr auto maximumArpeggioOctave = 10;    // 10×12 = 120, only a few notes missed.
    static constexpr auto minimumArpeggioOctave = -maximumArpeggioOctave;
    static constexpr auto minimumArpeggio = (-12 * maximumArpeggioOctave) + 1;       // Same for software and hardware.
    static constexpr auto maximumArpeggio = (12 * maximumArpeggioOctave) - 1;
    static constexpr auto minimumPitch = -0xfff;         // Same for software and hardware. NOT for Expressions!
    static constexpr auto maximumPitch = 0xfff;
    static constexpr auto minimumSpeed = 0;
    static constexpr auto maximumSpeed = 255;

    static constexpr auto hardwareVolumeValue = 16;                  // Volume value indicating the sound is hardware.

    static constexpr auto maskNoise = static_cast<unsigned int>(maximumNoise);  // Mask to apply to keep only the noise.
    static constexpr auto maskVolume = 0b11111U;                     // Mask to apply to keep only the volume (hardware envelope included).
    static constexpr auto maskVolumeHardwareVolumeFlag = 0b10000U;   // Mask to apply to a volume to keep only the hardware envelope flag.
    static constexpr auto maskHardwareEnvelope = 0xfU;               // Mask to apply to keep only the hardware envelope.

    static constexpr auto maskSoftwarePeriodLSB = 0xffU;                    // Mask to apply to keep only the LSB of a software period.
    static constexpr auto maskSoftwarePeriodMSB = 0xfU;                     // Mask to apply to keep only the mSB of a software period.
    static constexpr auto maskSoftwarePeriod = 0xfffU;
    static constexpr auto maskHardwarePeriodLSB = 0xffU;                    // Mask to apply to keep only the LSB of a hardware period.
    static constexpr auto maskHardwarePeriodMSB = 0xffU;                    // Mask to apply to keep only the mSB of a hardware period.
    static constexpr auto maskHardwarePeriod = 0xffffU;

    static const float cpcStereoChannelMiddleRatio;             // How the second channel is ratioed, on a CPC, if stereo.

    static const float minimumSampleAmplificationRatio;
    static const float maximumSampleAmplificationRatio;

    static const int cpcPsgLeftChannelVolumePercent;
    static const int cpcPsgCenterChannelVolumePercent;
    static const int cpcPsgRightChannelVolumePercent;

    static const double sampleMultiplierMinimumValue;
    static const double sampleMultiplierMaximumValue;

    static constexpr auto defaultSidBalancePercent = 50;
    static constexpr auto defaultSidLowShelfVolume = 0;
    static constexpr auto defaultSidActivated = false;
    static constexpr auto defaultSidRatio = 2;
    static constexpr auto sidMinimumRatio = 0;
    static constexpr auto sidMaximumRatio = 4;
    static constexpr auto defaultSidArpeggio = 0;
    static constexpr auto sidMinimumArpeggio = 0;
    static constexpr auto sidMaximumArpeggio = 12;
    static constexpr auto defaultSidPitch = 0;
    static constexpr auto sidMinimumPitch = -24;
    static constexpr auto sidMaximumPitch = 24;

    static constexpr auto defaultSidPlayerFrequencyHz = 312 * 50;
    static constexpr auto sidPlayerMinimumFrequencyHz = 2000;
    static constexpr auto sidPlayerMaximumFrequencyHz = 44100;
    static constexpr auto sidMinimumPeriod = 1;
    static constexpr auto sidMaximumPeriod = maximumSoftwarePeriod;

    static constexpr auto minimumPercent = 0;                   // Generic...
    static constexpr auto maximumPercent = 100;
};

}   // namespace arkostracker
