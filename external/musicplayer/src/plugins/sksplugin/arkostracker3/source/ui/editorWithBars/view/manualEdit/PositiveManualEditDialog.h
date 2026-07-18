#pragma once

#include "BaseManualEditDialog.h"

namespace arkostracker
{

/** A concrete edit dialog with a positive hexadecimal number. */
class PositiveManualEditDialog final : public BaseManualEditDialog
{
public:
    /**
     * Constructor.
     * @param title the title to display.
     * @param minimumValue the minimum value, for display.
     * @param maximumValue the maximum value, for display.
     * @param validateCallback called when OK is clicked, will validate or not the given String. The returned String, if not empty, is an error message.
     * The client may close this Dialog if it considers the entry valid.
     * @param closeCallback called when dialog can be closed.
     * @param mainText a possible main text override.
     * @param width the width of the window.
     */
    PositiveManualEditDialog(const juce::String& title, int minimumValue, int maximumValue, const std::function<juce::String(juce::String)>& validateCallback,
        const std::function<void()>& closeCallback,
        const juce::String& mainText = juce::String(), int width = 450) noexcept;
};

}   // namespace arkostracker
