#include "PositiveManualEditDialog.h"

#include "../../../../utils/NumberUtil.h"

namespace arkostracker
{

PositiveManualEditDialog::PositiveManualEditDialog(const juce::String& title, const int pMinimumValue, const int pMaximumValue,
                                                   const std::function<juce::String(juce::String)>& pValidateCallback, const std::function<void()>& pCloseCallback,
                                                   const juce::String& pMainText, const int pWidth) noexcept :
    BaseManualEditDialog(title,
                         pMainText.isNotEmpty()
                             ? pMainText
                             : juce::translate("Enter hex values from " + NumberUtil::signedHexToStringWithPrefix(pMinimumValue, juce::String()).toLowerCase() +
                                               " to " + NumberUtil::signedHexToStringWithPrefix(pMaximumValue, juce::String()).toLowerCase() + ", separated by comma or space."),
                         pValidateCallback, pCloseCallback, pWidth, 210)
{
}

} // namespace arkostracker
