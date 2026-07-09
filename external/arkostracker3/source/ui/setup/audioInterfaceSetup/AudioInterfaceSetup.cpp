#include "AudioInterfaceSetup.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/AudioController.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/MidiController.h"

namespace arkostracker 
{

AudioInterfaceSetup::AudioInterfaceSetup(MainController& pMainController, std::function<void()> pReturnCallback) noexcept :
        ModalDialog(juce::translate("Audio/MIDI interfaces"), 600, 430,
                    [&] { onOkOrCancelButtonClicked(true); }, [&] { onOkOrCancelButtonClicked(false); }, true),
        mainController(pMainController),
        audioDeviceViewport(),
        audioDeviceSelectorComponent(pMainController.getAudioController().getAudioDeviceManager(),
                                     0, 0, 2, 2,
                                     true, false, true, false),
        toggleAddOctaveToInputMidiNotes(juce::translate("Add current octave to input MIDI notes (4 = +0 octave).")),
        returnCallback(std::move(pReturnCallback)),
        originalMustAddOctaveToInputMidiNotes()
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    const auto bounds = getUsableModalDialogBounds();
    const auto audioDeviceViewportBounds = bounds.withTrimmedBottom(labelsHeight + margins);
    audioDeviceViewport.setBounds(audioDeviceViewportBounds);
    audioDeviceSelectorComponent.setBounds(audioDeviceViewportBounds);    // We don't actually care, it will resize itself.
    toggleAddOctaveToInputMidiNotes.setBounds(bounds.getX(), audioDeviceViewport.getBottom() + margins, bounds.getWidth(), labelsHeight);

    audioDeviceViewport.setViewedComponent(&audioDeviceSelectorComponent, false);
    addComponentToModalDialog(audioDeviceViewport);
    addComponentToModalDialog(toggleAddOctaveToInputMidiNotes);

    const auto& preferences = PreferencesManager::getInstance();
    originalMustAddOctaveToInputMidiNotes = preferences.mustAddOctaveToInputMidiNotes();
    toggleAddOctaveToInputMidiNotes.setToggleState(originalMustAddOctaveToInputMidiNotes, juce::NotificationType::dontSendNotification);

    toggleAddOctaveToInputMidiNotes.setTooltip(juce::translate("When on, the selected octave (at the top of the main window), "
                                                               "is added to the MIDI note octave, with octave 4 meaning octave +0, 5 meaning +1, "
                                                               "6 +2 etc. Below octave 4, it gets negative: octave 3 means -1, octave 2 -2, etc."));
}

void AudioInterfaceSetup::onOkOrCancelButtonClicked(const bool ok) const noexcept
{
    // Stores the audio preferences.
    const auto& preferences = PreferencesManager::getInstance();
    preferences.saveAudioProperties(audioDeviceSelectorComponent);
    // Contrary to the audio, Midi has no callback when a change is performed. So updates it in all cases.
    mainController.getAudioController().getMidiController().scanMidiDevicesAndRegister();

    if (ok) {
        const auto newMustAddOctaveToInputMidiNotes = toggleAddOctaveToInputMidiNotes.getToggleState();
        if (newMustAddOctaveToInputMidiNotes != originalMustAddOctaveToInputMidiNotes) {
            preferences.setMustAddOctaveToInputMidiNotes(newMustAddOctaveToInputMidiNotes);

            // Notifies.
            mainController.observers().getGeneralDataObservers().applyOnObservers([&] (GeneralDataObserver* observer) {
                observer->onGeneralDataChanged(GeneralDataObserver::What::mustAddOctaveToMidiInputNoteChanged);
            });
        }
    }

    returnCallback();
}

}   // namespace arkostracker
