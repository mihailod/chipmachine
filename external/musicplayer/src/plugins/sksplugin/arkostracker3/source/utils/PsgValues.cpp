#include "PsgValues.h"

#include "NumberUtil.h"

namespace arkostracker 
{

const float PsgValues::cpcStereoChannelMiddleRatio = 0.91F;         // Measured by Zik.

const float PsgValues::minimumSampleAmplificationRatio = 0.1F;
const float PsgValues::maximumSampleAmplificationRatio = 6.0F;

const int PsgValues::minimumHardwareEnvelope = 8;

const int PsgValues::cpcPsgLeftChannelVolumePercent = 100;
const int PsgValues::cpcPsgCenterChannelVolumePercent = 91;
const int PsgValues::cpcPsgRightChannelVolumePercent = 100;

const double PsgValues::sampleMultiplierMinimumValue = 0.1;
const double PsgValues::sampleMultiplierMaximumValue = 5.0;

int PsgValues::getPsgCount(const int pChannelCount) noexcept
{
    return (pChannelCount / channelCountPerPsg) + (((pChannelCount % channelCountPerPsg) == 0) ? 0 : 1);
}

int PsgValues::convertEnvelopeCurveToAt(const int envelope) noexcept
{
    // FIXME Put it bacK? jassert((envelope >= minimumHardwareEnvelope) && (envelope <= maximumHardwareEnvelope));
    // From 8, AT3 manages it.
    if (envelope >= 8) {
        return NumberUtil::correctNumber(envelope, minimumHardwareEnvelope, maximumHardwareEnvelope);
    }
    auto unsignedEnv = static_cast<unsigned int>(envelope);

    // See http://www.cpcwiki.eu/index.php/PSG#0Dh_-_Volume_Envelope_Shape_.284bit.29
    if ((unsignedEnv & 0b1100U) == 0b0000U) {
        return 9;
    }
    if ((unsignedEnv & 0b1100U) == 0b0100U) {
        return 0xf;
    }

    jassertfalse;       // Shouldn't happen.
    return 8;           // Fallback value.
}

int PsgValues::getChannelCount(const int psgCount) noexcept
{
    return psgCount * channelCountPerPsg;
}

int PsgValues::getChannelIndex(const int channelIndex, const int psgIndex) noexcept
{
    jassert((channelIndex >= 0) && (channelIndex < channelCountPerPsg));
    jassert(psgIndex >= 0);
    return psgIndex * channelCountPerPsg + channelIndex;
}

int PsgValues::getPsgChannelIndex(const int channelIndex) noexcept
{
    return channelIndex % channelCountPerPsg;
}

int PsgValues::getPsgIndex(const int channelIndex) noexcept
{
    return channelIndex / channelCountPerPsg;
}

std::pair<int, int> PsgValues::getPsgAndChannelIndexes(const int channelIndexInSong) noexcept
{
    const auto psgIndex = channelIndexInSong / channelCountPerPsg;
    const auto channelIndexInPsg = channelIndexInSong % channelCountPerPsg;

    return { psgIndex, channelIndexInPsg };
}

std::pair<int, int> PsgValues::getFirstAndLastChannels(const int psgIndex) noexcept
{
    const auto firstChannel = getChannelIndex(0, psgIndex);
    const auto lastChannel = firstChannel + channelCountPerPsg - 1;
    return { firstChannel, lastChannel };
}

std::vector<int> PsgValues::getChannelsFromPsg(const int psgIndex) noexcept
{
    const auto firstChannel = getChannelIndex(0, psgIndex);

    return { firstChannel, firstChannel + 1, firstChannel + 2 };
}

}   // namespace arkostracker
