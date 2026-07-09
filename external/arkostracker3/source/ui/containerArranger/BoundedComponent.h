#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** An abstract pure class for Component which children must display inside with a border. */
class BoundedComponent : public juce::Component
{
public:
    /** The X, inside the parent component. */
    virtual int getXInsideComponent() const noexcept = 0;
    /** The Y, inside the parent component. */
    virtual int getYInsideComponent() const noexcept = 0;
    /** The width to use inside the parent component. */
    virtual int getAvailableWidthInComponent() const noexcept = 0;
    /** The height to use inside the parent component. */
    virtual int getAvailableHeightInComponent() const noexcept = 0;
};

}   // namespace arkostracker
