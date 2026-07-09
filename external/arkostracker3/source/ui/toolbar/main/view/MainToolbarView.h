#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Abstract view of the Main Toolbar. */
class MainToolbarView : public juce::Component
{
public:
    enum class PlayButton
    {
        playFromStart,
        play,
        playPatternFromStart,
        playPattern,
        stop,
    };

    /**
     * Shows the timer to display.
     * @param timer the timer ("-5:23" for example).
     */
    virtual void setTimerToDisplay(juce::String timer) noexcept = 0;

    /**
     * Sets the arrangement to highlight.
     * @param number the number (>=0).
     */
    virtual void setCurrentArrangementNumber(int number) noexcept = 0;

    /**
     * Sets the shown octave.
     * @param octave the octave.
     */
    virtual void setOctave(int octave) noexcept = 0;

    /**
     * Sets the speed.
     * @param speed the speed.
     * @param bpm the bpm, or 0 if not found.
     */
    virtual void setSpeed(int speed, int bpm) noexcept = 0;

    /**
     * Changes the serial icon state.
     * @param connected true if connected.
     * @param enabled true if the button is enabled.
     */
    virtual void setSerialIconState(bool connected, bool enabled) noexcept = 0;

    /**
     * Displays the bubble on the serial Button. If empty, nothing is shown.
     * @param message the message to display. May be empty.
     */
    virtual void showSerialBubble(const juce::String& message) noexcept = 0;

    /**
     * Sets play the button to highlight.
     * @param playButtonToHighlight the button to highlight.
     */
    virtual void setHighlightedPlayIcon(MainToolbarView::PlayButton playButtonToHighlight) noexcept = 0;
};

}   // namespace arkostracker

