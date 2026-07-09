#include "CustomDocumentWindow.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

CustomDocumentWindow::CustomDocumentWindow(const juce::String& name, const bool useOpenGl) noexcept :
        DocumentWindow(name, juce::Colours::lightgrey, allButtons),
        openGlContext(),
        tooltipWindow(nullptr),

        isNativeTitleBarUsed()     // Set below.
{
    // Reads the configuration, first to know if the native bar should be used.
    const auto& preferencesManager = PreferencesManager::getInstance();
    isNativeTitleBarUsed = preferencesManager.isNativeTitleBarUsed();

    // Should use OpenGL?
    if (useOpenGl && preferencesManager.mustUseOpenGl()) {
        openGlContext.attachTo(*this);
    }

    setUsingNativeTitleBar(isNativeTitleBarUsed);
}

CustomDocumentWindow::~CustomDocumentWindow()
{
    if (openGlContext.isAttached()) {
        openGlContext.detach();
    }
}

juce::BorderSize<int> CustomDocumentWindow::getBorderThickness()
{
    // If the native title bar is used, let the system manages its own border thickness.
    if (isNativeTitleBarUsed) {
        return DocumentWindow::getBorderThickness();
    }

    // This is the small border around the application to allow it to be resized, if fullscreen.
    return juce::BorderSize(isFullScreen() ? 0 : LookAndFeelConstants::borderThickness);
}

void CustomDocumentWindow::setUp(Component& component, const int minimumWidth, const int minimumHeight) noexcept {

    setVisible(true);
    setResizable(true, false);
    setResizeLimits(minimumWidth, minimumHeight, 99999, 99999);
    centreWithSize(getWidth(), getHeight());

    // Links the ToolTip to the main window (and not nullptr), this corrects the Win7/Aero bug on tooltips.
    tooltipWindow = std::make_unique<juce::TooltipWindow>(&component);
}

}   // namespace arkostracker
