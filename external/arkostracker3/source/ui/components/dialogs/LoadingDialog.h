#pragma once


#include "ModalDialog.h"

namespace arkostracker 
{

/** A dialog with only a text for loading (or saving...) (no progress, no cancel button). */
class LoadingDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param title the title to display.
     * @param text the text, above the text field.
     * @param width the window width.
     * @param height the window height.
     */
    explicit LoadingDialog(const juce::String& title = juce::translate("Loading"), const juce::String& text = juce::translate("Please wait..."),
                           int width = 440, int height = 120) noexcept;

private:
    juce::Label label;
};

}   // namespace arkostracker
