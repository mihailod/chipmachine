#include "StoredOutputMix.h"

#include "../../../utils/PsgValues.h"

#include <juce_core/juce_core.h>

namespace arkostracker
{

const int StoredOutputMix::maximumVolumePercent = 100;

StoredOutputMix::StoredOutputMix(const int pGlobalVolume, const StereoMixType pStereoMixType, const int pCustomVolumeLeft,
                                 const int pCustomVolumeCenter, const int pCustomVolumeRight, const bool pEmulateSpeaker,
                                 const int pStereoSeparation) noexcept:
        globalVolume(pGlobalVolume),
        stereoMixType(pStereoMixType),
        customVolumeLeft(pCustomVolumeLeft),
        customVolumeCenter(pCustomVolumeCenter),
        customVolumeRight(pCustomVolumeRight),
        emulateSpeaker(pEmulateSpeaker),
        stereoSeparation(pStereoSeparation)
{
}

StoredOutputMix::StoredOutputMix() noexcept :
        globalVolume(maximumVolumePercent),
        stereoMixType(StereoMixType::flat),
        customVolumeLeft(maximumVolumePercent),
        customVolumeCenter(maximumVolumePercent),
        customVolumeRight(maximumVolumePercent),
        emulateSpeaker(false),
        stereoSeparation(maximumVolumePercent)
{
}

int StoredOutputMix::getGlobalVolume() const noexcept
{
    return globalVolume;
}

StoredOutputMix::StereoMixType StoredOutputMix::getStereoMixType() const noexcept
{
    return stereoMixType;
}

int StoredOutputMix::getCustomVolumeLeft() const noexcept
{
    return customVolumeLeft;
}

int StoredOutputMix::getCustomVolumeCenter() const noexcept
{
    return customVolumeCenter;
}

int StoredOutputMix::getCustomVolumeRight() const noexcept
{
    return customVolumeRight;
}

bool StoredOutputMix::isSpeakerEmulated() const noexcept
{
    return emulateSpeaker;
}

int StoredOutputMix::getStereoSeparation() const noexcept
{
    return stereoSeparation;
}

OutputMix StoredOutputMix::toOutputMix() const noexcept
{
    int leftVolume;         // NOLINT(*-init-variables)
    int centerVolume;       // NOLINT(*-init-variables)
    int rightVolume;        // NOLINT(*-init-variables)

    switch (stereoMixType) {
        case StereoMixType::cpc: {
            leftVolume = PsgValues::cpcPsgLeftChannelVolumePercent;
            centerVolume = PsgValues::cpcPsgCenterChannelVolumePercent;
            rightVolume = PsgValues::cpcPsgRightChannelVolumePercent;
            break;
        }
        case StereoMixType::custom: {
            leftVolume = customVolumeLeft;
            centerVolume = customVolumeCenter;
            rightVolume = customVolumeRight;
            break;
        }
        default:
            jassertfalse;
            [[fallthrough]];
        case StereoMixType::flat: {
            leftVolume = maximumVolumePercent;
            centerVolume = maximumVolumePercent;
            rightVolume = maximumVolumePercent;
            break;
        }
    }

    return {
        globalVolume,
        leftVolume,
        centerVolume,
        rightVolume,
        emulateSpeaker,
        stereoSeparation
    };
}

}   // namespace arkostracker
