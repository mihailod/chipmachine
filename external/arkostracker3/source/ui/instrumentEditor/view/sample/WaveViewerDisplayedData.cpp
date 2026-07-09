#include "WaveViewerDisplayedData.h"

#include <juce_core/juce_core.h>

#include "../../../../utils/PsgValues.h"

namespace arkostracker
{
WaveViewerDisplayedData::WaveViewerDisplayedData() noexcept :
        multiplier(0.0),
        startViewedOffset(0),
        pastEndViewedOffset(0),
        sampleLength(0),
        sampleFrequencyHz(0),
        looping(false),
        digidrumNote(PsgValues::digidrumNote)
{
}

WaveViewerDisplayedData::WaveViewerDisplayedData(const double pMultiplier, const int pStartViewedOffset, const int pPastEndViewedOffset, const int pSampleLength,
                                                 const int pSampleFrequencyHz, const bool pLooping, const int pDigidrumNote) noexcept :
        multiplier(pMultiplier),
        startViewedOffset(pStartViewedOffset),
        pastEndViewedOffset(pPastEndViewedOffset),
        sampleLength(pSampleLength),
        sampleFrequencyHz(pSampleFrequencyHz),
        looping(pLooping),
        digidrumNote(pDigidrumNote)
{
}

bool WaveViewerDisplayedData::isDisplayValid() const noexcept
{
    return (multiplier > 0.0) && (pastEndViewedOffset > 0) && (pastEndViewedOffset <= sampleLength) && (sampleLength > 0) && (sampleFrequencyHz > 0);
}

double WaveViewerDisplayedData::getMultiplier() const noexcept
{
    return multiplier;
}

int WaveViewerDisplayedData::getStartViewedOffset() const noexcept
{
    return startViewedOffset;
}

int WaveViewerDisplayedData::getPastEndViewedOffset() const noexcept
{
    return pastEndViewedOffset;
}

int WaveViewerDisplayedData::getSampleLength() const noexcept
{
    return sampleLength;
}

int WaveViewerDisplayedData::getSampleFrequencyHz() const noexcept
{
    return sampleFrequencyHz;
}

bool WaveViewerDisplayedData::isLooping() const noexcept
{
    return looping;
}

int WaveViewerDisplayedData::getDigidrumNote() const noexcept
{
    return digidrumNote;
}

bool WaveViewerDisplayedData::operator==(const WaveViewerDisplayedData& rhs) const
{
    return juce::exactlyEqual(multiplier, rhs.multiplier) &&
           startViewedOffset == rhs.startViewedOffset &&
           pastEndViewedOffset == rhs.pastEndViewedOffset &&
           sampleLength == rhs.pastEndViewedOffset &&
           sampleFrequencyHz == rhs.sampleFrequencyHz &&
           looping == rhs.looping &&
           digidrumNote == rhs.digidrumNote
    ;
}

bool WaveViewerDisplayedData::operator!=(const WaveViewerDisplayedData& rhs) const
{
    return !(rhs == *this);
}

}   // namespace arkostracker
