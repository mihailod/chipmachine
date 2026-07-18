#pragma once

#include "MainToolbarView.h"

namespace arkostracker 
{

/** A View of the Main toolbar that does nothing. */
class MainToolbarViewNoOp final : public MainToolbarView
{
public:
    void setTimerToDisplay(juce::String timer) noexcept override;
    void setCurrentArrangementNumber(int number) noexcept override;
    void setOctave(int octave) noexcept override;
    void setSpeed(int speed, int bpm) noexcept override;
    void setSerialIconState(bool connected, bool enabled) noexcept override;
    void showSerialBubble(const juce::String& message) noexcept override;
    void setHighlightedPlayIcon(PlayButton playButtonToHighlight) noexcept override;
};



}   // namespace arkostracker

