#include "OutputMix.h"

namespace arkostracker 
{

OutputMix::OutputMix(const int pGlobalVolume, const int pChannelAVolume, const int pChannelBVolume, const int pChannelCVolume,
                     const bool pEmulateSpeaker, const int pStereoSeparation) noexcept:
        globalVolume(pGlobalVolume),
        channelAVolume(pChannelAVolume),
        channelBVolume(pChannelBVolume),
        channelCVolume(pChannelCVolume),
        emulateSpeaker(pEmulateSpeaker),
        stereoSeparation(pStereoSeparation)
{
}

OutputMix OutputMix::buildDefault() noexcept
{
    return { 100, 100, 100, 100, false, 100 };
}

int OutputMix::getGlobalVolume() const noexcept
{
    return globalVolume;
}

int OutputMix::getChannelAVolume() const noexcept
{
    return channelAVolume;
}

int OutputMix::getChannelBVolume() const noexcept
{
    return channelBVolume;
}

int OutputMix::getChannelCVolume() const noexcept
{
    return channelCVolume;
}

bool OutputMix::isSpeakerEmulated() const noexcept
{
    return emulateSpeaker;
}

int OutputMix::getStereoSeparation() const noexcept
{
    return stereoSeparation;
}

bool OutputMix::operator==(const OutputMix& rhs) const
{
    return globalVolume == rhs.globalVolume &&
           channelAVolume == rhs.channelAVolume &&
           channelBVolume == rhs.channelBVolume &&
           channelCVolume == rhs.channelCVolume &&
           emulateSpeaker == rhs.emulateSpeaker &&
           stereoSeparation == rhs.stereoSeparation;
}

bool OutputMix::operator!=(const OutputMix& rhs) const
{
    return !(rhs == *this);
}

}   // namespace arkostracker
