#pragma once

#include "../SpecialTrackHeader.h"

namespace arkostracker 
{

/** The header at the top of the Speed Track. */
class EventTrackHeader final : public SpecialTrackHeader
{
public:
    /**
     * Constructor.
     * @param listener the listener to the events.
     */
    explicit EventTrackHeader(Listener& listener) noexcept;

protected:
    juce::Colour getBackgroundColor() const noexcept override;
    const juce::Image& getImage() const noexcept override;

private:
    juce::Image image;
};

}   // namespace arkostracker

