#include "SelectedData.h"

namespace arkostracker
{

SelectedData::SelectedData(std::vector<CaptureFlags> pTrackToCaptureFlag, Location pStartLocation, const OptionalValue<Location> &pEndLocation,
                           const int pStartChannelIndex, const bool pIncludeSpeedTrack, const bool pIncludeEventTrack) noexcept :
        trackToCaptureFlag(std::move(pTrackToCaptureFlag)),
        startLocation(std::move(pStartLocation)),
        endLocation(pEndLocation),
        startChannelIndex(pStartChannelIndex),
        includeSpeedTrack(pIncludeSpeedTrack),
        includeEventTrack(pIncludeEventTrack)
{
}

const std::vector<CaptureFlags> &SelectedData::getTrackToCaptureFlag() const noexcept
{
    return trackToCaptureFlag;
}

const Location& SelectedData::getStartLocation() const noexcept
{
    return startLocation;
}

const OptionalValue<Location>& SelectedData::getEndLocation() const noexcept
{
    return endLocation;
}

int SelectedData::getStartChannelIndex() const noexcept
{
    return startChannelIndex;
}

bool SelectedData::getIncludeSpeedTrack() const noexcept
{
    return includeSpeedTrack;
}

bool SelectedData::getIncludeEventTrack() const noexcept
{
    return includeEventTrack;
}

std::set<int> SelectedData::getSelectedChannelIndexes() const noexcept
{
    std::set<int> trackIndexes;

    for (auto offset = 0; offset < static_cast<int>(trackToCaptureFlag.size()); ++offset) {
        trackIndexes.insert(startChannelIndex + offset);
    }

    return trackIndexes;
}

SelectedData::SelectedColumn SelectedData::getSelectedColumns() const noexcept
{
    auto isNoteCaptured = false;
    auto isInstrumentCaptured = false;
    auto isEffectCaptured = false;
    auto isSpeedTrackCaptured = getIncludeSpeedTrack();
    auto isEventTrackCaptured = getIncludeEventTrack();
    for (const auto& captureFlag : trackToCaptureFlag) {
        isNoteCaptured |= captureFlag.isNoteCaptured();
        isInstrumentCaptured |= captureFlag.isInstrumentCaptured();
        isEffectCaptured |= captureFlag.isEffect1Captured() || captureFlag.isEffect2Captured() || captureFlag.isEffect3Captured() || captureFlag.isEffect4Captured();
    }

    return { isNoteCaptured, isInstrumentCaptured, isEffectCaptured, isSpeedTrackCaptured, isEventTrackCaptured };
}

}   // namespace arkostracker
