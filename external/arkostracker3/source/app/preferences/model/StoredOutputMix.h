#pragma once

#include <cstdint>

#include "../../../controllers/model/OutputMix.h"

namespace arkostracker
{

/** Stores how the final output mix is performed. */
class StoredOutputMix
{
public:
    enum class StereoMixType : std::uint8_t {
        flat,
        cpc,
        custom,
    };

    /**
     * Constructor.
     * @param globalVolume the global volume, from 0 to 100.
     * @param stereoMixType how the mix is performed.
     * @param customVolumeLeft the custom left volume, from 0 to 100.
     * @param customVolumeCenter the custom center volume, from 0 to 100.
     * @param customVolumeRight the custom right volume, from 0 to 100.
     * @param emulateSpeaker true to emulate the speaker.
     * @param stereoSeparation the separation, from 0 (mono) to 100 (full stereo).
     */
    StoredOutputMix(int globalVolume, StereoMixType stereoMixType, int customVolumeLeft, int customVolumeCenter,
                    int customVolumeRight, bool emulateSpeaker, int stereoSeparation) noexcept;

    /** A default constructor. Useful for optional. */
    StoredOutputMix() noexcept;

    /** Converts a stored output mix to an output mix.*/
    OutputMix toOutputMix() const noexcept;

    int getGlobalVolume() const noexcept;
    StereoMixType getStereoMixType() const noexcept;
    int getCustomVolumeLeft() const noexcept;
    int getCustomVolumeCenter() const noexcept;
    int getCustomVolumeRight() const noexcept;
    bool isSpeakerEmulated() const noexcept;
    int getStereoSeparation() const noexcept;

private:
    static const int maximumVolumePercent;

    int globalVolume;
    StereoMixType stereoMixType;
    int customVolumeLeft;
    int customVolumeCenter;
    int customVolumeRight;
    bool emulateSpeaker;
    int stereoSeparation;
};

}   // namespace arkostracker
