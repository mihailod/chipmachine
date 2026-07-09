#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * Shows a Marker (a name with a small vertical line).
 * The component takes care of its own size, only set its X/Y.
 */
class MarkerView final : public juce::Component
{
public:
    static constexpr int desiredHeight = 40;

    /**
     * Constructor.
     * @param name the marker name. It should not be empty, else the marker shouldn't be present.
     */
    explicit MarkerView(juce::String name) noexcept;

    // Component method implementations.
    // ================================================
    void paint(juce::Graphics& g) override;

private:
    static const float fontHeight;
    static const float dashLengths[];                   // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays)

    juce::Font font;
    juce::String name;
};

}   // namespace arkostracker
