#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../utils/OptionalValue.h"

namespace arkostracker
{

/**
 * A simple holder of BarArea, to put in a Viewport.
 * Only useful to show Components over the children.
 */
class BarAreaHolder : public juce::Component
{
public:
    /** Constructor. */
    BarAreaHolder() noexcept;

    /** Sets the loop, and updates the UI. */
    void setLoop(int startBarX, int endBarX, bool isLooping, int barY) noexcept;

    // juce::Component method implementations.
    // ============================================
    void paintOverChildren(juce::Graphics& g) override;
    void lookAndFeelChanged() override;

private:
    static const float dashLengths[];                   // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays)
    static const float barAlpha;

    static void drawLine(const juce::Graphics& g, float x, float y, float endY) noexcept;

    int startBarX;
    int endBarX;
    bool isLooping;
    int barY;

    OptionalValue<juce::Colour> barColor;
};

}       // namespace arkostracker
