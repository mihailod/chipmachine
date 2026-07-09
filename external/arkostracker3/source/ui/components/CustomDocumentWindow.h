#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

namespace arkostracker 
{

/**
 * Use this class as any DocumentWindow to share some initialization code.
 * The client must call the setUp method.
 */
class CustomDocumentWindow : public juce::DocumentWindow
{
public:
    /**
     * Constructor.
     * @param name the name to display at the top.
     * @param useOpenGl true to use OpenGL. It seems using more than one is problematic! So only the main window should...
     */
    explicit CustomDocumentWindow(const juce::String& name, bool useOpenGl) noexcept;

    ~CustomDocumentWindow() override;

    // DocumentWindow method implementations.
    // ================================================
    juce::BorderSize<int> getBorderThickness() override;

    // ================================================
    /**
     * Sets up the document window.
     * @param component the Component to attach the tooltip to. Should be the "main component" of this Document Window.
     * @param minimumWidth the minimum width this windows can have.
     * @param minimumHeight the minimum width this windows can have.
     */
    void setUp(Component& component, int minimumWidth, int minimumHeight) noexcept;

private:
    juce::OpenGLContext openGlContext;                            // An OpenGL Context to display the UI inside.
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;           // To have Tooltips. Only one instance is needed.
    bool isNativeTitleBarUsed;                                    // True if the native title bar is used.
};

}   // namespace arkostracker
