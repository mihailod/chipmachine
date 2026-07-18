#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Abstract class for the Splash dialogs. */
class SplashDialog
{
public:
    /** Destructor. */
    virtual ~SplashDialog() = default;

    /** Shows the splash screen. */
    virtual void show() noexcept = 0;

protected:
    static constexpr int versionLabelWidth = 100;

    /** Sets up the version label, which is given. This does not locate it! */
    static void setUpVersionLabel(juce::Label& versionLabel) noexcept;
};

}   // namespace arkostracker
