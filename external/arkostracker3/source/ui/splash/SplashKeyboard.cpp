#include "SplashKeyboard.h"

#include <BinaryData.h>

#include "../lookAndFeel/LookAndFeelConstants.h"
#include "../../app/preferences/PreferencesManager.h"
#include "../setup/keyboardMapping/KeyboardMappingSetter.h"
#include "../../controllers/MainController.h"
#include "../lookAndFeel/LookAndFeelFactory.h"
#include "../components/LookAndFeelChanger.h"

namespace arkostracker 
{

SplashKeyboard::SplashKeyboard(LookAndFeelChanger& pLookAndFeelChanger, MainController& pMainController, std::function<void()> pCallback) noexcept :
        CustomResizableWindow("Splash"),
        lookAndFeelChanger(pLookAndFeelChanger),
        mainController(pMainController),
        callback(std::move(pCallback)),

        contentComponent(),
        logoImage(),
        versionLabel(),
        welcomeLabel(juce::String(), juce::translate("Welcome to Arkos Tracker 3!")),
        explanationLabel(juce::String(), juce::translate("First of all, please choose your keyboard layout.\nIt will still be possible to change it later in the settings.")),
        keyboardLayoutsComboBox(),
        okButton(juce::translate("Go!")),

        themeLabel(juce::String(), juce::translate("Select a theme. You will be able to change it\nor create your own later in the settings.")),
        themeChooser(),

        virtualKeyboardLayoutNames(std::make_unique<VirtualKeyboardLayoutNames>())
{
}


// SplashDialog method implementations.
// ==========================================================

void SplashKeyboard::show() noexcept
{
    const int desiredWidth = 500;
    const int desiredHeight = 520;

    setOpaque(true);

    logoImage.setImage(juce::ImageFileFormat::loadFrom(BinaryData::Logo512_png, BinaryData::Logo512_pngSize), juce::RectanglePlacement::centred);
    setUpVersionLabel(versionLabel);

    okButton.onClick = [&] { exitSplash(); };

    centreWithSize(desiredWidth, desiredHeight);
    contentComponent.setSize(desiredWidth, desiredHeight);
    setContentNonOwned(&contentComponent, false);

    // Populates the keyboard layout.
    const auto currentKeyboardLayout = PreferencesManager::getInstance().getVirtualKeyboardLayout();
    virtualKeyboardLayoutNames->fillComboBoxWithKeyboardLayoutNames(keyboardLayoutsComboBox, currentKeyboardLayout);

    // Populates the themes.
    themeChooser.clear(juce::NotificationType::dontSendNotification);
    for (const auto themeId : LookAndFeelFactory::getNativeThemeIds()) {
        const auto theme = LookAndFeelFactory::retrieveLookAndFeel(themeId);
        themeChooser.addItem(theme->displayedName, themeId);
    }
    const auto currentThemeId = PreferencesManager::getInstance().getCurrentThemeId();
    themeChooser.setSelectedId(currentThemeId, juce::NotificationType::dontSendNotification);
    themeChooser.addListener(this);

    contentComponent.addAndMakeVisible(&logoImage);
    contentComponent.addAndMakeVisible(&versionLabel);
    contentComponent.addAndMakeVisible(&welcomeLabel);
    contentComponent.addAndMakeVisible(&explanationLabel);
    contentComponent.addAndMakeVisible(&keyboardLayoutsComboBox);
    contentComponent.addAndMakeVisible(&okButton);
    contentComponent.addAndMakeVisible(&themeLabel);
    contentComponent.addAndMakeVisible(&themeChooser);

    enterModalState(true, nullptr, false);
    setVisible(true);
}


// Component method implementations.
// ==========================================================

void SplashKeyboard::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto logoDimension = 200;
    const auto labelHeights = 25;
    const auto verticalMargin = margins;
    const auto okButtonWidth = 90;
    const auto left = margins;

    // First, the logo at the top.
    logoImage.setBounds((width - logoDimension) / 2, 20, logoDimension, logoDimension);

    const auto titleHeight = 30;
    const auto titleSize = 34.0F;
    const auto explanationHeight = 3 * 18;
    const auto comboBoxWidth = 160;

    versionLabel.setBounds(width - versionLabelWidth - margins, verticalMargin, versionLabelWidth, labelHeights);

    // The big text at the top.
    welcomeLabel.setBounds(0, logoImage.getBottom() + margins, width, titleHeight);
    welcomeLabel.setJustificationType(juce::Justification::centred);
    welcomeLabel.getProperties().set(LookAndFeelConstants::labelPropertyFontSize, titleSize);

    explanationLabel.setBounds(0, welcomeLabel.getBottom() + margins, width, explanationHeight);
    explanationLabel.setJustificationType(juce::Justification::centred);
    keyboardLayoutsComboBox.setBounds((width - comboBoxWidth) / 2, explanationLabel.getBottom(), comboBoxWidth, labelHeights);

    themeLabel.setBounds(left, keyboardLayoutsComboBox.getBottom() + margins, width - left * 2, explanationHeight);
    themeChooser.setBounds((width - comboBoxWidth) / 2, themeLabel.getBottom(), comboBoxWidth, labelHeights);
    themeLabel.setJustificationType(juce::Justification::centred);

    okButton.setBounds((width - okButtonWidth) / 2, height - labelHeights - verticalMargin, okButtonWidth, labelHeights);

    ResizableWindow::resized();         // Required by the parent.
}

bool SplashKeyboard::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey) {
        exitSplash();
        return true;
    }

    return Component::keyPressed(key);
}

// juce::ComboBox::Listener method implementations.
// ==========================================================

void SplashKeyboard::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &themeChooser) {
        applyAndStoreTheme(themeChooser.getSelectedId());
    } else {
        jassertfalse;       // Unmanaged ComboBox?
    }
}


// ==========================================================

void SplashKeyboard::exitSplash() noexcept
{
    // Gets and stores the layout, but mostly changes the keyboard commands.
    auto& commandManager = mainController.getCommandManager();
    const auto keyboardLayout = getSelectedKeyboardLayout();
    KeyboardMappingSetter::changeKeyboardLayout(commandManager, keyboardLayout);

    // Notifies the listener.
    callback();
}

VirtualKeyboardLayout SplashKeyboard::getSelectedKeyboardLayout() const noexcept
{
    // The index of the selected index can be directly linked to the enumeration.
    const auto keyboardIndex = keyboardLayoutsComboBox.getSelectedItemIndex();
    jassert((keyboardIndex >= static_cast<int>(VirtualKeyboardLayout::first)) && (keyboardIndex <= static_cast<int>(VirtualKeyboardLayout::last)));

    return static_cast<VirtualKeyboardLayout>(keyboardIndex);
}

void SplashKeyboard::applyAndStoreTheme(int themeId) noexcept
{
    PreferencesManager::getInstance().setCurrentThemeId(themeId);
    lookAndFeelChanger.setLookAndFeel(themeId);
}


}   // namespace arkostracker

