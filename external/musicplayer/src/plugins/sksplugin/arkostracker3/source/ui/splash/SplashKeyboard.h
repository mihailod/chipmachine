#pragma once

#include "SplashDialog.h"
#include "../keyboard/VirtualKeyboardLayout.h"
#include "../components/dialogs/CustomResizableWindow.h"

namespace arkostracker 
{

class MainController;
class LookAndFeelChanger;

/** Splash shown if the keyboard layout isn't known (and allows to change the theme as a bonus !). On confirmation, the layout is stored in the Preferences. */
class SplashKeyboard : public SplashDialog,
                       public CustomResizableWindow,
                       public juce::ComboBox::Listener
{
public:
    /**
     * Constructor.
     * @param lookAndFeelChanger object that knows how to change the theme.
     * @param mainController the Main controller.
     * @param callback the callback when done.
     */
    explicit SplashKeyboard(LookAndFeelChanger& lookAndFeelChanger, MainController& mainController, std::function<void()> callback) noexcept;

    // SplashDialog method implementations.
    // ==========================================================
    void show() noexcept override;

    // Component method implementations.
    // ==========================================================
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    // juce::ComboBox::Listener method implementations.
    // ==========================================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

private:
    static const auto margins = 15;

    /** Saves the "show on startup" flag and calls the callback. */
    void exitSplash() noexcept;

    /** @return the keyboard layout selected in this Dialog. */
    VirtualKeyboardLayout getSelectedKeyboardLayout() const noexcept;

    /**
     * Applies the Theme which ID is given, and stores it in the Preferences.
     * @param themeId the ID of the theme. Must be valid.
     */
    void applyAndStoreTheme(int themeId) noexcept;

    LookAndFeelChanger& lookAndFeelChanger;
    MainController& mainController;
    std::function<void()> callback;

    juce::Component contentComponent;                   // Where all the views are added.
    juce::ImageComponent logoImage;
    juce::Label versionLabel;
    juce::Label welcomeLabel;
    juce::Label explanationLabel;
    juce::ComboBox keyboardLayoutsComboBox;
    juce::TextButton okButton;

    juce::Label themeLabel;
    juce::ComboBox themeChooser;

    std::unique_ptr<VirtualKeyboardLayoutNames> virtualKeyboardLayoutNames;           // Allows to populate the Combobox with the keyboard layout names.
};


}   // namespace arkostracker

