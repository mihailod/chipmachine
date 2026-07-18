#include "PatternViewerViewNoOp.h"

namespace arkostracker 
{

void PatternViewerViewNoOp::setDisplayedMetadata(const PatternViewerView::DisplayedMetadata& /*displayedMetadata*/,
                                                 const PatternViewerView::DisplayedData& /*displayedData*/) noexcept
{
}

void PatternViewerViewNoOp::setDisplayedData(const PatternViewerView::DisplayedData& /*displayedData*/) noexcept
{
}

int PatternViewerViewNoOp::getViewportX() const noexcept
{
    return 0;
}

void PatternViewerViewNoOp::setViewportX(int /*x*/) noexcept
{
}

void PatternViewerViewNoOp::setShownLineAndCursorLocation(int /*line*/, OptionalValue<CursorLocation> /*cursorLocation*/) noexcept
{
}

void PatternViewerViewNoOp::updateMeters(std::unordered_map<int, PeriodAndNoiseMeterInput> /*psgIndexToPeriodAndNoiseMeterInput*/) noexcept
{
}

void PatternViewerViewNoOp::setRecordingState(bool /*newRecordState*/) noexcept
{
}

void PatternViewerViewNoOp::setFollowMode(Follow /*newFollowMode*/) noexcept
{
}

OptionalBool PatternViewerViewNoOp::determineOutOfBoundsDirectionFromMouseEvent(const juce::MouseEvent& /*mouseEvent*/) noexcept
{
    return { };
}

void PatternViewerViewNoOp::setSteps(int /*steps*/) noexcept
{
}

void PatternViewerViewNoOp::setWriteInstrumentState(bool /*writeInstrumentOnWriteNote*/) noexcept
{
}

void PatternViewerViewNoOp::setInformationText(const juce::String& /*text*/, bool /*isError*/) noexcept
{
}

void PatternViewerViewNoOp::setOverwriteDefaultEffect(bool /*locked*/) noexcept
{
}

void PatternViewerViewNoOp::setDefaultEffect(Effect /*effect*/) noexcept
{
}

void PatternViewerViewNoOp::onUserWantsToSetMusicTrackNameAndColor(int /*channelIndex*/, const juce::String& /*currentName*/, OptionalValue<juce::Colour> /*currentColor*/) noexcept
{
}

void PatternViewerViewNoOp::onUserWantsToSetSpecialTrackName(bool /*isSpeedTrack*/, const juce::String& /*currentName*/) noexcept
{
}

void PatternViewerViewNoOp::onUserWantsToSetLink(int /*channelIndex*/) noexcept
{
}

void PatternViewerViewNoOp::onUserWantsToSetSpecialLink(bool /*isSpeedTrack*/) noexcept
{
}

void PatternViewerViewNoOp::setEffectContextText(int /*channelIndex*/, const juce::String& /*text*/) noexcept
{
}

void PatternViewerViewNoOp::showWarningBecauseRecordingOff() noexcept
{
}

}   // namespace arkostracker
