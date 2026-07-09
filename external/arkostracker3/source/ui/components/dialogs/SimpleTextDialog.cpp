#include "SimpleTextDialog.h"

namespace arkostracker 
{

SimpleTextDialog::SimpleTextDialog(const juce::String& pTitle, const juce::String& pText,
                                   std::function<void()> pOkCallback, std::function<void()> pCancelCallback,
                                   int pWidth, int pHeight, bool pShowOkButton, bool pShowCancelButton) noexcept :
        ModalDialog(pTitle, pWidth, pHeight, std::move(pOkCallback), std::move(pCancelCallback),
                    pShowOkButton, pShowCancelButton, false),
        label(juce::String(), pText)
{
    label.setBounds(getUsableModalDialogBounds());
    label.setJustificationType(juce::Justification::centred);

    addComponentToModalDialog(label);
}

}   // namespace arkostracker

