#pragma once

#include <functional>

#include "ModalDialog.h"

namespace arkostracker 
{

/** Dialog showing a text and OK/Cancel button. */
class SimpleTextDialog : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param title the title to display.
     * @param text the text, above the text field.
     * @param okCallback the callback to call when the OK button is clicked.
     * @param cancelCallback the callback to call when the Cancel button is clicked.
     * @param width the window width.
     * @param height the window height.
     * @param showOkButton true to show the OK button.
     * @param showCancelButton true to show the cancel button.
     */
    SimpleTextDialog(const juce::String& title, const juce::String& text,
                     std::function<void()> okCallback, std::function<void()> cancelCallback,
                     int width = 500, int height = 120, bool showOkButton = true, bool showCancelButton = true) noexcept;

private:
    juce::Label label;
};

}   // namespace arkostracker

