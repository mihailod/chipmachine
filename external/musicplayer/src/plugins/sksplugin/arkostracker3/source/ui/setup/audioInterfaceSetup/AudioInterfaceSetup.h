#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class MainController;

/** Shows the audio interface setup (sound card, midi, etc.). */
class AudioInterfaceSetup final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param mainController to sent event.
     * @param returnCallback called when the OK or close Button is clicked.
     */
    AudioInterfaceSetup(MainController& mainController, std::function<void()> returnCallback) noexcept;

private:
    /** Called when the OK or Cancel Button is clicked. */
    void onOkOrCancelButtonClicked(bool ok) const noexcept;

    MainController& mainController;
    juce::Viewport audioDeviceViewport;                                     // Viewport around the Audio Devices, it may have to scroll.
    juce::AudioDeviceSelectorComponent audioDeviceSelectorComponent;        // To select the Audio Devices.
    juce::ToggleButton toggleAddOctaveToInputMidiNotes;

    std::function<void()> returnCallback;

    bool originalMustAddOctaveToInputMidiNotes;
};

}   // namespace arkostracker
