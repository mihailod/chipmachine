#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A ProgressBar with a text in the middle. */
class ProgressBarView final : public juce::Component
{
public:
    /** Constructor. */
    ProgressBarView() noexcept;

    /**
     * Sets the maximum progress value. This does not refresh the UI.
     * @param maximumProgressValue the maximum value.
     */
    void setMaximumProgressValue(int maximumProgressValue) noexcept;

    /**
     * Sets the value to display. The maximum value should have been set before (but 100 is the default value). This calls for a repaint.
     * If the value is less than 0 or more than the maximum, it is corrected.
     * @param currentProgressValue the value to show, proportionally to the maximum one, set before.
     */
    void setProgressValue(int currentProgressValue) noexcept;


    // Component method overrides.
    // =====================================================
    void paint(juce::Graphics& g) override;

private:
    int maximumProgressValue;               // The maximum value to display. 100 by default.
    int progressPercent;                    // The progress, in percent.
    juce::Colour foregroundColor;           // The color in the bar.
    juce::Colour textColor;                 // The color in the bar.
};

}   // namespace arkostracker

