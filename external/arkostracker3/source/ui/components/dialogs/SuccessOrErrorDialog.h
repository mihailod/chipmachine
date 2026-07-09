#pragma once

#include "ModalDialog.h"

namespace arkostracker 
{

/** Simple dialog with a text and an OK button. */
class SuccessOrErrorDialog final : public ModalDialog
{
public:
    static constexpr auto defaultWidth = 500;
    static constexpr auto defaultHeight = 120;

    /**
     * Builds a Dialog for a success.
     * @param text the text to show.
     * @param callback the callback called when OK is clicked.
     * @param width the width of the window.
     * @param height the height of the window.
     */
    static std::unique_ptr<SuccessOrErrorDialog> buildForSuccess(const juce::String& text, const std::function<void()>& callback,
                                                                int width = defaultWidth, int height = defaultHeight) noexcept;

    /**
     * Builds a Dialog for an error.
     * @param text the text to show.
     * @param callback the callback called when OK is clicked.
     * @param width the width of the window.
     * @param height the height of the window.
     */
    static std::unique_ptr<SuccessOrErrorDialog> buildForError(const juce::String& text, const std::function<void()>& callback,
                                                               int width = defaultWidth, int height = defaultHeight) noexcept;

    /**
     * Constructor. Better use the other static builders.
     * @param text the text, above the text field.
     * @param callback the callback to call when exiting.
     * @param title the title to display.
     * @param width the width.
     * @param height the height.
     */
    SuccessOrErrorDialog(const juce::String& text, const std::function<void()>& callback, const juce::String& title,
                         int width, int height) noexcept;

private:
    juce::Label label;
};

}   // namespace arkostracker
