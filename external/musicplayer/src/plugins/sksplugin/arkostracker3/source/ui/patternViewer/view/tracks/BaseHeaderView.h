#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * A base class for the header at the top of a LineTrack/SpecialTrack.
 * It provides the background.
 */
class BaseHeaderView : public juce::Component
{
public:
    /** Constructor. */
    BaseHeaderView() noexcept;

    // Component method implementations.
    // =====================================
    void resized() override;
    void paint(juce::Graphics& g) override;

protected:
    /**
     * Called after the background is painted.
     * @param g the Graphics object.
     * @param backgroundColor the background color used, as a convenience.
     */
    virtual void paintAfter(juce::Graphics& g, juce::Colour backgroundColor) noexcept = 0;

    /** @return the color to fill the background with. */
    virtual juce::Colour getBackgroundColor() const noexcept = 0;

private:
    static const float cornerSize;

    juce::Path backgroundPath;                  // The cached path of the background.
};

}   // namespace arkostracker
