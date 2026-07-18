#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** The abstract view for the "none selected" Instrument Editor. */
class NoInstrumentEditorView : public juce::Component
{
public:

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;
};

}   // namespace arkostracker

