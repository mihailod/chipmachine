#include "SongNotSavedDialog.h"

namespace arkostracker 
{

SongNotSavedDialog::SongNotSavedDialog(const std::function<void()>& onSaveClicked, const std::function<void()>& onDontSaveClicked,
                                       const std::function<void()>& onCancelClicked) noexcept :
        ModalDialog(juce::translate("Save changes"), 420, 150,
                    [=] { onSaveClicked(); },
                    [=] { onCancelClicked(); },
                    true, true),
        textLabel(juce::String(), juce::translate("Do you want to save the changes to your song?")),
        dontSaveButton(juce::translate("Don't save"))
{
    setOkButtonText(juce::translate("Save"));

    const auto bounds = getUsableModalDialogBounds();
    addComponentToModalDialog(textLabel);
    addComponentToModalDialog(dontSaveButton);
    textLabel.setBounds(bounds);
    textLabel.setJustificationType(juce::Justification::centred);
    const auto middleX = bounds.getWidth() / 2 + bounds.getX();

    dontSaveButton.onClick = [=] { onDontSaveClicked(); };

    // Relocates the buttons.
    constexpr auto okButtonWidth = 90;
    constexpr auto dontSaveButtonWidth = 110;
    const auto buttonsY = getButtonsY();
    const auto buttonsHeight = getButtonsHeight();
    constexpr auto separator = 50;

    dontSaveButton.setBounds(middleX - dontSaveButtonWidth / 2, buttonsY, dontSaveButtonWidth, buttonsHeight);
    setOkButtonX(dontSaveButton.getX() - okButtonWidth - separator);
    setOkButtonWidth(okButtonWidth);
    setCancelButtonX(dontSaveButton.getRight() + separator);
}

}   // namespace arkostracker
