#include "RelativeManualEditDialog.h"

#include "../../../../utils/NumberUtil.h"
#include "../../../utils/TextEditorUtil.h"

namespace arkostracker
{

RelativeManualEditDialog::RelativeManualEditDialog(const juce::String& title, const int pMinimumValue, const int pMaximumValue,
                                                   const std::function<juce::String(juce::String)>& pValidateCallback,
                                                   const std::function<void()>& pCloseCallback) noexcept :
        BaseManualEditDialog(title,
            juce::translate("Enter hex values from " + NumberUtil::signedHexToStringWithPrefix(pMinimumValue, juce::String()).toLowerCase() +
                " to " + NumberUtil::signedHexToStringWithPrefix(pMaximumValue, juce::String()).toLowerCase() + ", separated by comma or space."),
            pValidateCallback, pCloseCallback, 450,210)
{
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> RelativeManualEditDialog::getTextEditorRestriction() noexcept
{
    return TextEditorUtil::buildRestrictionForQuickEditRelative(characterMaximumCount);
}

}   // namespace arkostracker
