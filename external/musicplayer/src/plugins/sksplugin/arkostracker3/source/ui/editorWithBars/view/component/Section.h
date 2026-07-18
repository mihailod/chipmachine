#pragma once

#include "../../../components/AutoSizeLabel.h"
#include "../../../components/BlankComponent.h"

namespace arkostracker 
{

/** A section, consisting of a Label above a line. */
class Section final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param text the text to display.
     */
    explicit Section(const juce::String& text) noexcept;

    /** @return the static height of this Component. */
    static int getSectionHeight() noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    static const int separatorHeight;
    static const float separatorAlpha;

    AutoSizeLabel label;
    BlankComponent separatorLeft;
    BlankComponent separatorRight;
};

}   // namespace arkostracker
