#include "SuccessOrErrorDialog.h"

namespace arkostracker 
{

SuccessOrErrorDialog::SuccessOrErrorDialog(const juce::String& text, const std::function<void()>& callback, const juce::String& title,
    const int width, const int height) noexcept :
        ModalDialog(title, width, height, callback, callback, true, false, false),
        label(juce::String(), text)
{
    label.setBounds(getUsableModalDialogBounds());
    label.setJustificationType(juce::Justification::centred);

    addComponentToModalDialog(label);
}

std::unique_ptr<SuccessOrErrorDialog> SuccessOrErrorDialog::buildForSuccess(const juce::String& text, const std::function<void()>& callback,
    const int width, const int height) noexcept
{
    return std::make_unique<SuccessOrErrorDialog>(text, callback, juce::translate("Success"), width, height);
}

std::unique_ptr<SuccessOrErrorDialog> SuccessOrErrorDialog::buildForError(const juce::String& text, const std::function<void()>& callback,
    const int width, const int height) noexcept
{
    return std::make_unique<SuccessOrErrorDialog>(text, callback, juce::translate("Error"), width, height);
}

}   // namespace arkostracker
