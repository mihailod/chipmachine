#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Data about what is displayed. */
class BarCaptionDisplayedData
{
public:
    /**
     * Constructor, with either an image and/or a text.
     * @param pImage the image, or empty.
     * @param pMainText the main text, or empty.
     * @param pIgnored true if this value is not used (because not used in this sound type for example).
     * @param pOutOfBounds true if out of bounds (after length).
     * @param pWithinLoop true if equal or before the loop end.
     * @param pGenerated true if generated (spread).
     */
    BarCaptionDisplayedData(juce::Image pImage, juce::String pMainText, const bool pIgnored, const bool pOutOfBounds, const bool pWithinLoop, const bool pGenerated) :
            image(std::move(pImage)),
            mainText(std::move(pMainText)),
            ignored(pIgnored),
            outOfBounds(pOutOfBounds),
            withinLoop(pWithinLoop),
            generated(pGenerated)
    {
    }

    /** Constructor with nothing shown. */
    BarCaptionDisplayedData() :
            image(juce::Image()),
            mainText(),
            ignored(false),
            outOfBounds(false),
            withinLoop(false),
            generated(false)
    {
    }

    const juce::Image& getImage() const noexcept
    {
        return image;
    }

    const juce::String& getMainText() const noexcept
    {
        return mainText;
    }

    bool isIgnored() const noexcept
    {
        return ignored;
    }

    bool isOutOfBounds() const noexcept
    {
        return outOfBounds;
    }

    bool isWithinLoop() const noexcept
    {
        return withinLoop;
    }

    bool isGenerated() const noexcept
    {
        return generated;
    }

    bool operator==(const BarCaptionDisplayedData& rhs) const noexcept
    {
        return image == rhs.image &&
                mainText == rhs.mainText &&
                ignored == rhs.ignored &&
                outOfBounds == rhs.outOfBounds &&
                withinLoop == rhs.withinLoop &&
                generated == rhs.generated;
    }

    bool operator!=(const BarCaptionDisplayedData& rhs) const noexcept
    {
        return !(rhs == *this);
    }

private:
    juce::Image image;
    juce::String mainText;
    bool ignored;
    bool outOfBounds;
    bool withinLoop;
    bool generated;
};

}   // namespace arkostracker

