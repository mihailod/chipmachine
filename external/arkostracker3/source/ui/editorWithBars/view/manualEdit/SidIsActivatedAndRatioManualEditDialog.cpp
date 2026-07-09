#include "SidIsActivatedAndRatioManualEditDialog.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

const juce::String SidIsActivatedAndRatioManualEditDialog::textEditorRestriction("-01234 ,");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

SidIsActivatedAndRatioManualEditDialog::SidIsActivatedAndRatioManualEditDialog(const int minimumValue, const int maximumValue, const std::function<juce::String(juce::String)>& pValidateCallback,
    const std::function<void()>& pCloseCallback) noexcept :
        BaseManualEditDialog(juce::translate("SID activation and ratio quick edit"),
            juce::translate("Enter \"-\" to stop SID, or a ratio from " + juce::String(minimumValue) + " to " + juce::String(maximumValue)
                + ", separated by comma or space."),
            pValidateCallback, pCloseCallback, 520,210)
{
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> SidIsActivatedAndRatioManualEditDialog::getTextEditorRestriction() noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(characterMaximumCount, textEditorRestriction);
}

}   // namespace arkostracker
