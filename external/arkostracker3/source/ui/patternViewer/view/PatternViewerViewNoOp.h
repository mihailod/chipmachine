#pragma once

#include <unordered_map>

#include "PatternViewerView.h"

namespace arkostracker 
{

/** An implementation of the PatternViewerView that does nothing. */
class PatternViewerViewNoOp final : public PatternViewerView
{
public:
    void setDisplayedMetadata(const DisplayedMetadata& displayedMetadata, const DisplayedData& displayedData) noexcept override;
    void setShownLineAndCursorLocation(int line, OptionalValue<CursorLocation> cursorLocation) noexcept override;
    void setDisplayedData(const DisplayedData& displayedData) noexcept override;
    int getViewportX() const noexcept override;
    void setViewportX(int x) noexcept override;
    void updateMeters(std::unordered_map<int, PeriodAndNoiseMeterInput> psgIndexToPeriodAndNoiseMeterInput) noexcept override;

    void setRecordingState(bool newRecordState) noexcept override;
    void setFollowMode(Follow newFollowMode) noexcept override;
    void setSteps(int steps) noexcept override;
    void setWriteInstrumentState(bool writeInstrumentOnWriteNote) noexcept override;
    void setOverwriteDefaultEffect(bool locked) noexcept override;
    void setDefaultEffect(Effect effect) noexcept override;

    OptionalBool determineOutOfBoundsDirectionFromMouseEvent(const juce::MouseEvent& mouseEvent) noexcept override;
    void setInformationText(const juce::String& text, bool isError) noexcept override;

    void onUserWantsToSetMusicTrackNameAndColor(int channelIndex, const juce::String& currentName, OptionalValue<juce::Colour> currentColor) noexcept override;
    void onUserWantsToSetSpecialTrackName(bool isSpeedTrack, const juce::String& currentName) noexcept override;
    void onUserWantsToSetLink(int channelIndex) noexcept override;
    void onUserWantsToSetSpecialLink(bool isSpeedTrack) noexcept override;

    void setEffectContextText(int channelIndex, const juce::String& text) noexcept override;
    void showWarningBecauseRecordingOff() noexcept override;
};

}   // namespace arkostracker

