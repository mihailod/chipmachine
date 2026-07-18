#include "SamplePlayInfo.h"

#include "../song/psg/PsgFrequency.h"

namespace arkostracker 
{

SamplePlayInfo::SamplePlayInfo() noexcept :
        empty(true),
        sample(nullptr),
        amplification(0.0F),
        loop(Loop()),
        sampleFrequencyHz(PsgFrequency::defaultSampleFrequencyHz),
        pitchHz(0.0F),
        referenceFrequencyHz(PsgFrequency::defaultReferenceFrequencyHz),
        volume4bits(0),
        playSampleFromStart(false),
        highPriority(false),
        forceSampleFrequencyForReplay(false)
{
}

SamplePlayInfo::SamplePlayInfo(std::shared_ptr<Sample> pSample, const bool pPlaySampleFromStart, const float pAmplification,
                               const Loop pLoop, const int pSampleFrequencyHz, const float pPitchHz, const float pReferenceFrequencyHz, const int pVolume4bits,
                               const bool pHighPriority, const bool pForceSampleFrequencyForReplay) noexcept :
        empty(false),
        sample(std::move(pSample)),
        amplification(pAmplification),
        loop(pLoop),
        sampleFrequencyHz(pSampleFrequencyHz),
        pitchHz(pPitchHz),
        referenceFrequencyHz(pReferenceFrequencyHz),
        volume4bits(pVolume4bits),
        playSampleFromStart(pPlaySampleFromStart),
        highPriority(pHighPriority),
        forceSampleFrequencyForReplay(pForceSampleFrequencyForReplay)
{
}

SamplePlayInfo SamplePlayInfo::buildNoSample() noexcept
{
    return { nullptr, false, 1.0F, Loop(),
                          PsgFrequency::defaultSampleFrequencyHz, 0.0F, PsgFrequency::defaultReferenceFrequencyHz,
                          0, true };     // High priority, else it won't cut the digidrums.
}

std::shared_ptr<Sample> SamplePlayInfo::getSample() const noexcept
{
    return sample;
}

bool SamplePlayInfo::isPlaySampleFromStart() const noexcept
{
    return playSampleFromStart;
}

bool SamplePlayInfo::isSamplePresent() const noexcept
{
    return (sample != nullptr);
}

float SamplePlayInfo::getAmplification() const noexcept
{
    return amplification;
}

const Loop& SamplePlayInfo::getLoop() const noexcept
{
    return loop;
}

int SamplePlayInfo::getSampleFrequencyHz() const noexcept
{
    return sampleFrequencyHz;
}

float SamplePlayInfo::getPitchHz() const noexcept
{
    return pitchHz;
}

int SamplePlayInfo::getVolume4Bits() const noexcept
{
    return volume4bits;
}

float SamplePlayInfo::getReferenceFrequency() const noexcept
{
    return referenceFrequencyHz;
}

bool SamplePlayInfo::isHighPriority() const noexcept
{
    return highPriority;
}

bool SamplePlayInfo::isEmpty() const
{
    return empty;
}

bool SamplePlayInfo::mustForceSampleFrequencyForReplay() const noexcept
{
    return forceSampleFrequencyForReplay;
}

}   // namespace arkostracker
