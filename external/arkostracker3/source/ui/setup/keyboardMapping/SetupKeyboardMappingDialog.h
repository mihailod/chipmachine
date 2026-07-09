#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../components/BlankComponent.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../keyboard/VirtualKeyboardLayout.h"

namespace arkostracker 
{

class MainController;

/**
 * Dialog to change the keyboard mapping. On exit (if OK), saves the keys in the Preferences.
 * Don't forget to declare all the Targets in the ApplicationCommandManager (main.cpp) to find them here!
 */
class SetupKeyboardMappingDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param mainController to get the ApplicationCommandManager.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
     */
    explicit SetupKeyboardMappingDialog(const MainController& mainController, std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

private:
    /** Called when the "change keyboard layout" is clicked. Prompts the user. */
    void onChangeKeyboardLayoutButtonClicked() noexcept;

    /**
     * Changes the keyboard layout. This also updates the preferences.
     * @param keyboardLayout the new layout.
     */
    void changeKeyboardLayout(VirtualKeyboardLayout keyboardLayout) const noexcept;

    /** @return the selected virtual keyboard layout from the combobox. */
    VirtualKeyboardLayout getSelectedLayoutFromCombobox() const noexcept;

    /** Called when the OK Button is clicked. */
    void onOkButtonClicked() const noexcept;

    juce::ApplicationCommandManager& applicationCommandManager;
    std::function<void()> okCallback;

    std::unique_ptr<juce::KeyMappingEditorComponent> keyMappingEditorComponent;         // Shows all the mapping.

    juce::Label changeKeyboardLayoutLabel;
    juce::ComboBox changeKeyboardLayoutComboBox;
    juce::TextButton changeKeyboardLayoutButton;                            // The button to validate.
    BlankComponent changeKeyboardBackground;                                // The background behind the "change keyboard layout" part.

    std::unique_ptr<juce::DialogWindow> confirmLayoutChangeDialog;          // Dialog for the user to confirm the keyboard layout change.
};

}   // namespace arkostracker
