#include "DcOffsetCalculator.h"

namespace arkostracker
{

DcOffsetCalculator::DcOffsetCalculator(const int pChannelCount) noexcept :
    channelCount(pChannelCount),
    totalAndWeightPerChannel()        // One entry per channel.
{
    totalAndWeightPerChannel.resize(static_cast<size_t>(pChannelCount));
}

void DcOffsetCalculator::readAudioSource(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) noexcept
{
    const auto startIndex = static_cast<size_t>(audioSourceChannelInfo.startSample);
    const auto sampleCount = audioSourceChannelInfo.numSamples;
    const auto pastEndIndex = static_cast<size_t>(sampleCount) + startIndex;

    const auto* buffers = audioSourceChannelInfo.buffer;
    jassert(channelCount <= buffers->getNumChannels());    // NOLINT(*)

    // Browses each channel.
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        const auto* bufferPt = buffers->getReadPointer(channelIndex);

        // Browses each sample, adds its value to the total.
        auto total = 0.0F;
        for (auto sampleIndex = startIndex; sampleIndex < pastEndIndex; ++sampleIndex, ++bufferPt) {  // NOLINT(clion-misra-cpp2008-5-18-1, *-pro-bounds-pointer-arithmetic)
            total += *bufferPt;
        }

        // Adds the total to the existing one (calculated from the previous iteration this method was called).
        const auto totalAndWeight = TotalAndWeight::buildFromCumulatedScores(total, sampleCount);
        const auto accumulatedTotalAndWeight = totalAndWeightPerChannel.at(static_cast<size_t>(channelIndex));
        const auto newTotalAndWeight = totalAndWeight + accumulatedTotalAndWeight;
        totalAndWeightPerChannel.at(static_cast<size_t>(channelIndex)) = newTotalAndWeight;
    }
}

float DcOffsetCalculator::getDcOffset(const int channelIndex) const noexcept
{
    const auto& totalAndWeight = totalAndWeightPerChannel.at(static_cast<size_t>(channelIndex));
    return totalAndWeight.getScore();
}

void DcOffsetCalculator::applyDcOffset(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) const noexcept
{
    const auto startIndex = static_cast<size_t>(audioSourceChannelInfo.startSample);
    const auto sampleCount = audioSourceChannelInfo.numSamples;
    const auto pastEndIndex = static_cast<size_t>(sampleCount) + startIndex;

    const auto buffers = audioSourceChannelInfo.buffer;
    jassert(channelCount == buffers->getNumChannels());
    jassert(totalAndWeightPerChannel.size() == static_cast<size_t>(buffers->getNumChannels()));

    // Browses each channel.
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        const auto dcOffset = getDcOffset(channelIndex);

        auto* bufferPt = buffers->getWritePointer(channelIndex);
        
        // Browses each sample, adds the DC offset.
        for (auto sampleIndex = startIndex; sampleIndex < pastEndIndex; ++sampleIndex, ++bufferPt) { // NOLINT(*-pro-bounds-pointer-arithmetic)
            const auto readValue = *bufferPt;
            *bufferPt = readValue - dcOffset;
        }
    }
}

}   // namespace arkostracker
