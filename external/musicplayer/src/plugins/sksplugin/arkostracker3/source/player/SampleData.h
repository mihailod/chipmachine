#pragma once

#include <unordered_map>

#include "SamplePlayInfo.h"

namespace arkostracker 
{

/** Represents the samples of a PSG to play. It may be used for none, one or more channels of one PSG. */
class SampleData
{
public:
    /**
     * Constructor.
     * @param mustStopAll true to mean that all samples must be stopped.
     */
    explicit SampleData(bool mustStopAll = false) noexcept;

    /**
     * Adds the data of a sample info. If marked as empty, nothing is inserted. If the channel already has a data, it is overwritten.
     * @param channelIndex the channel index (<3).
     * @param samplePlayInfo how to play the sample. The sample may actually not exist.
     */
    void addSamplePlayInfoIfNotEmpty(int channelIndex, SamplePlayInfo samplePlayInfo) noexcept;

    /**
     * Fills the unused slots with a "stop sample".
     * @param channelCount how many channels there are.
     */
    void fillUnusedWithStops(int channelCount) noexcept;

    /**
     * @return the Sample info, which may contain no sample.
     * @param channelIndex the channel index (<3).
     */
    SamplePlayInfo getSamplePlayInfo(int channelIndex) const noexcept;

    /**
     * @return true if there is any data for the given channel index.
     * @param channelIndex the channel index (<3).
     */
    bool isDataPresent(int channelIndex) const noexcept;

    /** @return true if all the samples must be stopped. */
    bool isMustStopAll() const noexcept;

private:
    std::unordered_map<int, SamplePlayInfo> channelIndexToSamplePlayInfo;
    bool mustStopAll;                       // Special flag to indicate that nothing must be played.
};

}   // namespace arkostracker
