#pragma once

namespace arkostracker 
{

/**
 * How the final output mix is performed.
 */
class OutputMix
{
public:
    /**
     * Constructor.
     * @param globalVolume the global volume, from 0 to 100.
     * @param channelAVolume the channel A volume, from 0 to 100.
     * @param channelBVolume the channel B volume, from 0 to 100.
     * @param channelCVolume the channel C volume, from 0 to 100.
     * @param emulateSpeaker true to emulate the speaker.
     * @param stereoSeparation the separation, from 0 (mono) to 100 (full stereo).
     */
    OutputMix(int globalVolume, int channelAVolume, int channelBVolume, int channelCVolume, bool emulateSpeaker, int stereoSeparation) noexcept;

    /** @return a default instance. */
    static OutputMix buildDefault() noexcept;

    int getGlobalVolume() const noexcept;
    int getChannelAVolume() const noexcept;
    int getChannelBVolume() const noexcept;
    int getChannelCVolume() const noexcept;
    bool isSpeakerEmulated() const noexcept;
    int getStereoSeparation() const noexcept;

    bool operator==(const OutputMix& rhs) const;
    bool operator!=(const OutputMix& rhs) const;

private:
    int globalVolume;
    int channelAVolume;
    int channelBVolume;
    int channelCVolume;
    bool emulateSpeaker;
    int stereoSeparation;
};

}   // namespace arkostracker
