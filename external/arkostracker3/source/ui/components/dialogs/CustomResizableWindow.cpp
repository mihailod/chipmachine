#include "CustomResizableWindow.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

CustomResizableWindow::CustomResizableWindow(const juce::String& title) noexcept :
        ResizableWindow(title,
            juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground)),
            true)
{
}


// DialogWindow methods override.
// ==============================================================================

void CustomResizableWindow::lookAndFeelChanged()
{
    // We have to do that for a real-time change of color.
    auto currentColor = getBackgroundColour();
    auto newColor = findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));
    if (currentColor != newColor) {
        setBackgroundColour(newColor);
    }
}


}   // namespace arkostracker

