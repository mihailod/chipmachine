#include "GeneralSetupDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../../app/preferences/PreferencesManager.h"

namespace arkostracker 
{

GeneralSetupDialog::GeneralSetupDialog(const std::function<void()>& pExitCallback) noexcept :
        ModalDialog(juce::translate("General properties"), 430, 280, [&] { validate(); }, [=] { pExitCallback(); },
                    true, true),
        exitCallback(pExitCallback),
        toggleShowSongInfoAfterLoad(juce::translate("Show song information after loading one.")),
        toggleUseNativeTitleBar(juce::translate("Use native title bar (restart to apply change).")),
        toggleUseOpenGl(juce::translate("Use OpenGL (restart to apply change).")),
        toggleCompressXml(juce::translate("Compress saved files (recommended).")),
        toggleShowSplash(juce::translate("Show splash screen on start-up.")),
        toggleForceNonNativeFileChooserWhenLoadInstrument(juce::translate("Force non-native file chooser when loading an instrument (tick this if you don't see any preview)."))
{
    for (auto* view : { &toggleShowSongInfoAfterLoad, &toggleUseNativeTitleBar, &toggleUseOpenGl, &toggleCompressXml, &toggleShowSplash,
            &toggleForceNonNativeFileChooserWhenLoadInstrument }) {
        addComponentToModalDialog(*view);
    }

    const auto bounds = getUsableModalDialogBounds();
    const auto x = bounds.getX();
    const auto y = bounds.getY();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto labelsHeightLarge = LookAndFeelConstants::labelsHeight + 20;
    constexpr auto separatorY = 4;

    toggleShowSongInfoAfterLoad.setBounds(x, y, width, labelsHeight);
    toggleShowSplash.setBounds(x, toggleShowSongInfoAfterLoad.getBottom() + separatorY, width, labelsHeight);
    toggleUseNativeTitleBar.setBounds(x, toggleShowSplash.getBottom() + separatorY, width, labelsHeight);
    toggleUseOpenGl.setBounds(x, toggleUseNativeTitleBar.getBottom() + separatorY, width, labelsHeight);
    toggleCompressXml.setBounds(x, toggleUseOpenGl.getBottom() + separatorY, width, labelsHeight);
    // A larger width is used (larger than the window!) else the text is squashed!
    toggleForceNonNativeFileChooserWhenLoadInstrument.setBounds(x, toggleCompressXml.getBottom() + separatorY, width, labelsHeightLarge);

    // Fills the data.
    const auto& preferences = PreferencesManager::getInstance();
    toggleShowSongInfoAfterLoad.setToggleState(preferences.isSongInfoShownAfterLoad(), juce::NotificationType::dontSendNotification);
    toggleShowSplash.setToggleState(preferences.isSplashShown(), juce::NotificationType::dontSendNotification);
    toggleUseNativeTitleBar.setToggleState(preferences.isNativeTitleBarUsed(), juce::NotificationType::dontSendNotification);
    toggleUseOpenGl.setToggleState(preferences.mustUseOpenGl(), juce::NotificationType::dontSendNotification);
    toggleCompressXml.setToggleState(preferences.areSavedFilesCompressed(), juce::NotificationType::dontSendNotification);
    toggleForceNonNativeFileChooserWhenLoadInstrument.setToggleState(preferences.isForceNonNativeFileChooserOnLoadInstrument(), juce::NotificationType::dontSendNotification);
}

void GeneralSetupDialog::validate() const noexcept
{
    // Saves the data.
    auto& preferences = PreferencesManager::getInstance();
    preferences.setShowSongInfoAfterLoad(toggleShowSongInfoAfterLoad.getToggleState());
    preferences.setShowSplash(toggleShowSplash.getToggleState());
    preferences.setNativeTitleBarUsed(toggleUseNativeTitleBar.getToggleState());
    preferences.setUseOpenGl(toggleUseOpenGl.getToggleState());
    preferences.setFilesCompressionOnSave(toggleCompressXml.getToggleState());
    preferences.setForceNonNativeFileChooserOnLoadInstrument(toggleForceNonNativeFileChooserWhenLoadInstrument.getToggleState());

    exitCallback();
}

}   // namespace arkostracker
