#include "PositiveDecimalManualEditDialog.h"

#include "../../../../utils/NumberUtil.h"
#include "../../../utils/TextEditorUtil.h"

namespace arkostracker
{

PositiveDecimalManualEditDialog::PositiveDecimalManualEditDialog(const juce::String& title, const int pMinimumValue, const int pMaximumValue,
                                                   const std::function<juce::String(juce::String)>& pValidateCallback, const std::function<void()>& pCloseCallback,
                                                   const juce::String& pMainText, const int pWidth) noexcept :
    BaseManualEditDialog(title,
                         pMainText.isNotEmpty()
                             ? pMainText
                             : juce::translate("Enter decimal values from " + juce::String(pMinimumValue) +
                                               " to " + juce::String(pMaximumValue) + ", separated by comma or space."),
                         pValidateCallback, pCloseCallback, pWidth, 210)
{
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> PositiveDecimalManualEditDialog::getTextEditorRestriction() noexcept
{
    const auto static restriction = TextEditorUtil::restrictionInt + " ,";
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(characterMaximumCount, restriction);
}

} // namespace arkostracker
