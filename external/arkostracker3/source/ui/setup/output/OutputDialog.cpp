#include "OutputDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/AudioController.h"
#include "../../../controllers/MainController.h"

namespace arkostracker 
{

OutputDialog::OutputDialog(MainController& pMainController, std::function<void()> pOkCallback, std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Output/mix"), 430, 370,
                    [&] { onOkButtonClicked(); },
                    std::move(pCancelCallback),
                    true, true),
        mainController(pMainController),
        okCallback(std::move(pOkCallback)),
        globalVolumeLabel(juce::String(), juce::translate("Global volume")),
        globalVolumeSlider(juce::String()),
        stereoSeparationLabel(juce::String(), juce::translate("Stereo separation")),
        stereoSeparationSlider(juce::String()),
        emulateSpeakerToggle(juce::translate("Emulate speaker")),
        outputMixGroup(*this)
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margin = LookAndFeelConstants::margins;
    constexpr auto sliderLabelsWidth = 150;
    const auto slidersWidth = width - sliderLabelsWidth;

    globalVolumeLabel.setBounds(left, top, sliderLabelsWidth, labelsHeight);
    globalVolumeSlider.setBounds(globalVolumeLabel.getRight(), globalVolumeLabel.getY(), slidersWidth, labelsHeight);
    globalVolumeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    globalVolumeSlider.setRange(0.0, 100.0, 1.0);

    stereoSeparationLabel.setBounds(left, globalVolumeLabel.getBottom() + margin, sliderLabelsWidth, labelsHeight);
    stereoSeparationSlider.setBounds(stereoSeparationLabel.getRight(), stereoSeparationLabel.getY(), slidersWidth, labelsHeight);

    emulateSpeakerToggle.setBounds(left, stereoSeparationLabel.getBottom() + margin, width, labelsHeight);
    outputMixGroup.locateItems(left, emulateSpeakerToggle.getBottom() + (margin * 4), width);   // Don't understand why such margin...

    for (auto* slider : { &stereoSeparationSlider, &globalVolumeSlider }) {
        slider->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        slider->setRange(0.0, 100.0, 1.0);
    }

    addComponentToModalDialog(globalVolumeLabel);
    addComponentToModalDialog(globalVolumeSlider);
    addComponentToModalDialog(stereoSeparationLabel);
    addComponentToModalDialog(stereoSeparationSlider);
    addComponentToModalDialog(emulateSpeakerToggle);

    fillUiFromStoredData();
}

void OutputDialog::onOkButtonClicked() const noexcept
{
    const auto data = outputMixGroup.retrieveData();

    // Saves the data.
    const StoredOutputMix storedOutputMix(
            static_cast<int>(globalVolumeSlider.getValue()),
            data.stereoMixType,
            data.customLeftVolumePercent,
            data.customCenterVolumePercent,
            data.customRightVolumePercent,
            emulateSpeakerToggle.getToggleState(),
            static_cast<int>(stereoSeparationSlider.getValue())
    );
    PreferencesManager::getInstance().storeOutputMix(storedOutputMix);

    // Applies the sound change.
    const auto outputMix = storedOutputMix.toOutputMix();
    mainController.getAudioController().setOutputMix(outputMix);

    okCallback();
}

void OutputDialog::fillUiFromStoredData() noexcept
{
    const auto outputMix = PreferencesManager::getInstance().getOutputMix();

    globalVolumeSlider.setValue(outputMix.getGlobalVolume(), juce::NotificationType::dontSendNotification);
    emulateSpeakerToggle.setToggleState(outputMix.isSpeakerEmulated(), juce::NotificationType::dontSendNotification);
    stereoSeparationSlider.setValue(outputMix.getStereoSeparation(), juce::NotificationType::dontSendNotification);
    outputMixGroup.fillUiFromData(outputMix.getCustomVolumeLeft(), outputMix.getCustomVolumeCenter(), outputMix.getCustomVolumeRight(), outputMix.getStereoMixType());
}

}   // namespace arkostracker
