#include "SampleData.h"

#include "../utils/PsgValues.h"

namespace arkostracker 
{

SampleData::SampleData(const bool pMustStopAll) noexcept :
        channelIndexToSamplePlayInfo(),
        mustStopAll(pMustStopAll)
{
}

void SampleData::addSamplePlayInfoIfNotEmpty(const int channelIndex, SamplePlayInfo samplePlayInfo) noexcept
{
    jassert(!mustStopAll);      // Illogical!
    jassert(channelIndex < PsgValues::channelCountPerPsg);

    if (!samplePlayInfo.isEmpty()) {
        channelIndexToSamplePlayInfo[channelIndex] = std::move(samplePlayInfo);
    }
}

SamplePlayInfo SampleData::getSamplePlayInfo(const int channelIndex) const noexcept
{
    jassert(channelIndex < PsgValues::channelCountPerPsg);

    // If no sample existing, returns an empty data.
    const auto iterator = channelIndexToSamplePlayInfo.find(channelIndex);
    if (iterator == channelIndexToSamplePlayInfo.cend()) {
        return { };
    }

    const auto& data = *iterator;
    return data.second;
}

void SampleData::fillUnusedWithStops(const int channelCount) noexcept
{
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        if (channelIndexToSamplePlayInfo.find(channelIndex) == channelIndexToSamplePlayInfo.cend()) {
            channelIndexToSamplePlayInfo[channelIndex] = SamplePlayInfo::buildNoSample();
        }
    }
}

bool SampleData::isDataPresent(const int channelIndex) const noexcept
{
    return channelIndexToSamplePlayInfo.find(channelIndex) != channelIndexToSamplePlayInfo.cend();
}

bool SampleData::isMustStopAll() const noexcept
{
    return mustStopAll;
}

}   // namespace arkostracker
