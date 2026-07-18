#include "SetupKeyboardMappingDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../components/dialogs/SimpleTextDialog.h"
#include "KeyboardMappingSetter.h"

namespace arkostracker 
{

SetupKeyboardMappingDialog::SetupKeyboardMappingDialog(const MainController& pMainController, std::function<void()> pOkCallback, std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Keyboard mapping"), 520, 500, [&] { onOkButtonClicked();},
                    std::move(pCancelCallback)),
        applicationCommandManager(pMainController.getCommandManager()),
        okCallback(std::move(pOkCallback)),
        keyMappingEditorComponent(),
        changeKeyboardLayoutLabel(juce::String(), juce::translate("Change the virtual keyboard layout (this will modify some keys above)")),
        changeKeyboardLayoutComboBox(),
        changeKeyboardLayoutButton(juce::translate("Overwrite")),
        changeKeyboardBackground(
                juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground))
                .contrasting(0.2F)),
        confirmLayoutChangeDialog()
{
    setOkButtonText(juce::translate("Save"));

    // Shows the Keyboard Component.
    keyMappingEditorComponent = std::make_unique<juce::KeyMappingEditorComponent>(*applicationCommandManager.getKeyMappings(), true);

    // From bottom to top.
    auto area = getUsableModalDialogBounds();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto halfMargins = margins / 2;
    constexpr auto buttonWidth = 100;

    changeKeyboardLayoutButton.onClick = [&] { onChangeKeyboardLayoutButtonClicked(); };

    // The ComboBox and the Button.
    area.removeFromBottom(margins);
    auto comboBoxAndButtonArea = area.removeFromBottom(labelsHeight);
    changeKeyboardLayoutButton.setBounds(comboBoxAndButtonArea.removeFromRight(buttonWidth));
    comboBoxAndButtonArea.removeFromRight(margins);
    changeKeyboardLayoutComboBox.setBounds(comboBoxAndButtonArea);

    // The label, then the keyboard editor.
    area.removeFromBottom(margins);
    changeKeyboardLayoutLabel.setBounds(area.removeFromBottom(labelsHeight));
    area.removeFromBottom(margins);
    keyMappingEditorComponent->setBounds(area);

    // Displays the background ot the Change Keyboard part.
    const auto changeKeyboardBackgroundHeight = changeKeyboardLayoutComboBox.getBottom() - changeKeyboardLayoutLabel.getY() + margins + halfMargins;
    changeKeyboardBackground.setBounds(changeKeyboardLayoutLabel.getX() - halfMargins, changeKeyboardLayoutLabel.getY() - halfMargins,
                                       changeKeyboardLayoutLabel.getWidth() + margins,
                                       changeKeyboardBackgroundHeight);

    // Fills the ComboBox with the layout names.
    // What is the currently used layout?
    const auto& preferencesManager = PreferencesManager::getInstance();
    const VirtualKeyboardLayout keyboardLayout = preferencesManager.getVirtualKeyboardLayout();
    // There should be one default layout, else the pop-up at the beginning would have filled it.
    jassert(keyboardLayout != VirtualKeyboardLayout::none);

    const VirtualKeyboardLayoutNames keyboardLayoutNames;
    keyboardLayoutNames.fillComboBoxWithKeyboardLayoutNames(changeKeyboardLayoutComboBox, keyboardLayout);

    addComponentToModalDialog(*keyMappingEditorComponent);
    addComponentToModalDialog(changeKeyboardBackground);
    addComponentToModalDialog(changeKeyboardLayoutLabel);
    addComponentToModalDialog(changeKeyboardLayoutComboBox);
    addComponentToModalDialog(changeKeyboardLayoutButton);
}

void SetupKeyboardMappingDialog::onOkButtonClicked() const noexcept
{
    // Saves the keyboard keys.
    auto& preferencesManager = PreferencesManager::getInstance();
    preferencesManager.storeKeyMapping(*keyMappingEditorComponent);

    // Notes that the Virtual KB selection is NOT saved, it is only saved when the Overwrite button is clicked.
    okCallback();
}

void SetupKeyboardMappingDialog::onChangeKeyboardLayoutButtonClicked() noexcept
{
    jassert(confirmLayoutChangeDialog == nullptr);          // Dialog already present?

    confirmLayoutChangeDialog = std::make_unique<SimpleTextDialog>(
            juce::translate("Confirm keyboard layout change"),
            juce::translate("This will overwrite the keys of the virtual keyboard. Proceed?"),
            [&] {
                changeKeyboardLayout(getSelectedLayoutFromCombobox());
                confirmLayoutChangeDialog.reset();
            },
            [&] { confirmLayoutChangeDialog.reset(); }
    );
}

VirtualKeyboardLayout SetupKeyboardMappingDialog::getSelectedLayoutFromCombobox() const noexcept
{
    auto selectedIndex = changeKeyboardLayoutComboBox.getSelectedItemIndex();
    return static_cast<VirtualKeyboardLayout>(selectedIndex);
}

void SetupKeyboardMappingDialog::changeKeyboardLayout(const VirtualKeyboardLayout keyboardLayout) const noexcept
{
    KeyboardMappingSetter::changeKeyboardLayout(applicationCommandManager, keyboardLayout);
}

}   // namespace arkostracker
