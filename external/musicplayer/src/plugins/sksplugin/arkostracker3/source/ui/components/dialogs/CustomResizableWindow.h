#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A simple JUCE Resizable Window, which take in account the look and feel change and a custom background color. */
class CustomResizableWindow : public juce::ResizableWindow
{
public:
    CustomResizableWindow(const juce::String& title) noexcept;

    // DialogWindow methods override.
    // ==============================================================================
    void lookAndFeelChanged() override;
};


}   // namespace arkostracker

