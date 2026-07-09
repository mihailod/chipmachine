#include "LoadingDialog.h"

namespace arkostracker 
{

LoadingDialog::LoadingDialog(const juce::String& title, const juce::String& text, const int width, const int height) noexcept :
        ModalDialog(title, width, height, nullptr, nullptr, false),
        label(juce::String(), text)
{
    const auto bounds = getUsableModalDialogBounds();
    label.setBounds(0, 0, bounds.getWidth(), bounds.getHeight());
    label.setJustificationType(juce::Justification::centred);

    addComponentToModalDialog(label);
}

}   // namespace arkostracker
