#pragma once

#include "BaseManualEditDialog.h"

namespace arkostracker
{

class SidIsActivatedAndRatioManualEditDialog final : public BaseManualEditDialog
{

public:
    /**
     * Constructor.
     * @param minimumValue the minimum value, for display.
     * @param maximumValue the maximum value, for display.
     * @param validateCallback called when OK is clicked, will validate or not the given String. The returned String, if not empty, is an error message.
     * The client may close this Dialog if it considers the entry valid.
     * @param closeCallback called when dialog can be closed.
     */
    SidIsActivatedAndRatioManualEditDialog(int minimumValue, int maximumValue, const std::function<juce::String(juce::String)>& validateCallback, const std::function<void()>& closeCallback) noexcept;

protected:
    std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> getTextEditorRestriction() noexcept override;

private:
    static const juce::String textEditorRestriction;
};

}   // namespace arkostracker
