#pragma once

#include "OutputMixGroup.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class MainController;

/**
 * Shows the output (mix) dialog (stereo separation, channel mix, etc.).
 */
class OutputDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param mainController to sent event.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
     */
    OutputDialog(MainController& mainController, std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

private:
    /** Fills the UI from the data stored in the Preferences. */
    void fillUiFromStoredData() noexcept;

    /** Called when the OK button is clicked. Saves the data and notifies the UI. */
    void onOkButtonClicked() const noexcept;

    MainController& mainController;
    std::function<void()> okCallback;

    juce::Label globalVolumeLabel;
    juce::Slider globalVolumeSlider;
    juce::Label stereoSeparationLabel;
    juce::Slider stereoSeparationSlider;
    juce::ToggleButton emulateSpeakerToggle;

    OutputMixGroup outputMixGroup;
};

}   // namespace arkostracker
