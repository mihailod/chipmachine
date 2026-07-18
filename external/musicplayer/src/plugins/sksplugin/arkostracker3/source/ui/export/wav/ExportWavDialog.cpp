#include "ExportWavDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/wav/SongWavExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/StringUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../utils/TextEditorUtil.h"

namespace arkostracker
{

int ExportWavDialog::minimumFrequencyHz = 4000;     // Arbitrary.

double ExportWavDialog::storedExportFrequencyHz = 44100.0;
int ExportWavDialog::storedAdditionalLoopCount = 0;

StoredOutputMix ExportWavDialog::storedOutputMix = StoredOutputMix(         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
    100,
    StoredOutputMix::StereoMixType::flat,
    100, 100, 100,
    false, 100
);
bool ExportWavDialog::storedExportChannelsIndividually = false;

ExportWavDialog::ExportWavDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to WAV"), 430, 470),
        subsongChooser(pMainController, {} ),
        exportFrequencyLabel(juce::String(), juce::translate("Output frequency (Hz)")),
        exportFrequencyEditText(juce::String(storedExportFrequencyHz)),
        stereoSeparationLabel(juce::String(), juce::translate("Stereo separation")),
        stereoSeparationSlider(juce::String()),
        outputMixGroup(*this),
        exportEachChannelIndividuallyToggle(juce::translate("Export each channel individually")),
        additionalLoopCountSlider(*this, juce::translate("Additional loop count"), 2, 1.0, 2.0, SliderIncDec::Filter::decimal, 150),

        restrictionHz(TextEditorUtil::buildRestrictionForInt(6)),
        progressTask(),
        dialog()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto exportFrequencyLabelWidth = 340;
    constexpr auto stereoSeparationLabelWidth = 190;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    exportFrequencyLabel.setBounds(left, subsongChooser.getBottom() + margins, exportFrequencyLabelWidth, labelsHeight);
    exportFrequencyEditText.setBounds(exportFrequencyLabel.getRight(), exportFrequencyLabel.getY(), width - exportFrequencyLabelWidth, labelsHeight);

    stereoSeparationLabel.setBounds(left, exportFrequencyEditText.getBottom() + margins, stereoSeparationLabelWidth, labelsHeight);
    stereoSeparationSlider.setBounds(stereoSeparationLabel.getRight(), stereoSeparationLabel.getY(), width - stereoSeparationLabelWidth, labelsHeight);
    const auto outputMixGroupY = stereoSeparationLabel.getBottom() + (4 * margins);
    outputMixGroup.locateItems(left, outputMixGroupY, width);
    exportEachChannelIndividuallyToggle.setBounds(left, outputMixGroupY + OutputMixGroup::mixGroupHeight - (2 * margins), width, labelsHeight);
    additionalLoopCountSlider.setBounds(left, exportEachChannelIndividuallyToggle.getBottom() + margins, width, labelsHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(exportFrequencyLabel);
    addComponentToModalDialog(exportFrequencyEditText);
    addComponentToModalDialog(stereoSeparationLabel);
    addComponentToModalDialog(stereoSeparationSlider);
    addComponentToModalDialog(exportEachChannelIndividuallyToggle);
    addComponentToModalDialog(additionalLoopCountSlider);

    stereoSeparationSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    stereoSeparationSlider.setRange(0.0, 100.0, 1.0);
    exportFrequencyEditText.setInputFilter(&restrictionHz, false);
    exportFrequencyEditText.onTextChange = [&] { onFrequencyEditTextChanged(); };

    // The values are only stored locally.
    outputMixGroup.fillUiFromData(storedOutputMix.getCustomVolumeLeft(), storedOutputMix.getCustomVolumeCenter(), storedOutputMix.getCustomVolumeRight(),
        storedOutputMix.getStereoMixType());
    additionalLoopCountSlider.setShownValue(storedAdditionalLoopCount);
    stereoSeparationSlider.setValue(storedOutputMix.getStereoSeparation(), juce::NotificationType::dontSendNotification);
    exportEachChannelIndividuallyToggle.setToggleState(storedExportChannelsIndividually, juce::NotificationType::dontSendNotification);
}

void ExportWavDialog::onCancelButtonClicked() const noexcept
{
    closeShownDialog();
}

void ExportWavDialog::closeShownDialog() const noexcept
{
    exit();
}

void ExportWavDialog::exit() const noexcept
{
    listener();
}

void ExportWavDialog::onExportButtonClicked() noexcept
{
    const auto valid = saveData();
    if (!valid) {
        jassertfalse;       // Should have been disabled by the UI.
        return;
    }

    const auto exportEachChannelIndividually = exportEachChannelIndividuallyToggle.getToggleState();

    // Opens the file picker.
    const auto fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::wav, FileExtensions::wavExtensionWithoutDot,
        !exportEachChannelIndividually);    // Delete the file only if it is the base file (if Export to several files, a target file must not be touched,
                                            // it will be appended to "channel - x", and it's these that must be deleted.
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto song = songController.getSong();

    // Creates the exporter, and the Task to perform it asynchronously.
    // The stored data can be directly used, they've been checked and stored before.
    const auto outputMix = storedOutputMix.toOutputMix();

    auto wavExporterTask = std::make_unique<SongWavExporter>(song, subsongId, storedExportFrequencyHz, storedAdditionalLoopCount, outputMix,
        fileToSaveTo, exportEachChannelIndividually);

    progressTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(juce::translate("Exporting..."),
        *this, std::move(wavExporterTask));
    progressTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportWavDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, std::unique_ptr<bool> /*result*/) noexcept
{
    progressTask.reset();

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }
    if (taskOutputState == TaskOutputState::finished) {
        dialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export to WAV finished!"), [&] { exit(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to WAV!"), [&] { closeShownDialog(); });
    }
}


// SliderIncDec::Listener method implementations.
// ======================================================

void ExportWavDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    if (&slider == &additionalLoopCountSlider) {
        const auto correctedValue = NumberUtil::correctNumber(value, 0.0, 99.0);
        slider.setShownValue(correctedValue);
    } else {
        jassertfalse;
    }
}

// ======================================================

bool ExportWavDialog::saveData() const noexcept
{
    // Is the frequency ok?
    const auto [isValid, exportFrequencyHz] = getFrequencyHz();
    if (!isValid) {
        return false;
    }

    // Stores the data locally.
    storedAdditionalLoopCount = static_cast<int>(additionalLoopCountSlider.getValue());
    storedExportFrequencyHz = exportFrequencyHz;
    const auto stereoSeparation = static_cast<int>(stereoSeparationSlider.getValue());

    const auto data = outputMixGroup.retrieveData();
    storedOutputMix = StoredOutputMix(
        storedOutputMix.getGlobalVolume(),
        data.stereoMixType,
        data.customLeftVolumePercent,
        data.customCenterVolumePercent,
        data.customRightVolumePercent,
        false,
        stereoSeparation
    );
    storedExportChannelsIndividually = exportEachChannelIndividuallyToggle.getToggleState();

    return true;
}

std::pair<bool, int> ExportWavDialog::getFrequencyHz() const noexcept
{
    auto success = false;
    const auto exportFrequencyHz = StringUtil::stringToInt(exportFrequencyEditText.getText(), success);
    if (!success) {
        return { false, 0 };
    }
    return { (exportFrequencyHz >= minimumFrequencyHz), exportFrequencyHz };
}

void ExportWavDialog::onFrequencyEditTextChanged() noexcept
{
    // Is the frequency ok?
    const auto [isValid, _] = getFrequencyHz();
    setOkButtonEnable(isValid);
}

}   // namespace arkostracker
