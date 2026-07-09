#include "ExportFapDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/ExportConfiguration.h"
#include "../../../export/fap/FapExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/StringUtil.h"
#include "../../ProjectInfo.h"          // Generated file in the build folder.
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

ExportFapDialog::ExportFapDialog(const MainController& pMainController, std::function<void()> pListener) noexcept:
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to FAP"), 400, 535),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& nameAndId) {
            onSubsongChanged(nameAndId.second);
        }),
        sourceLabelsPrefixLabel(juce::String(), juce::translate("Source constant label prefix (may be empty):")),
        sourceLabelsPrefixEditor(),
        psgFrequencyChooser(true, juce::translate("PSG frequency")),
        warningLabel(juce::String(), juce::translate("Warning! Only the first PSG will be exported!")),
        versionLabel(juce::String(), juce::translate("FAP version: ") + projectinfo::fapVersion),
        sourceConfiguration(PreferencesManager::getInstance().getCurrentSourceProfile().getSourceGeneratorConfiguration()),
        backgroundTask(),
        fileToSaveTo(),
        dialog()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    sourceLabelsPrefixLabel.setBounds(left, subsongChooser.getBottom() + margins, width, labelsHeight);
    sourceLabelsPrefixEditor.setBounds(left, sourceLabelsPrefixLabel.getBottom(), width, labelsHeight);

    const auto forcedPsgFrequencyHz = PreferencesManager::getInstance().getFapOutputFrequencyHz();
    psgFrequencyChooser.setFrequencyHz(forcedPsgFrequencyHz);
    psgFrequencyChooser.setBounds(left, sourceLabelsPrefixEditor.getBottom() + (margins * 2), width, PsgFrequencyChooser::desiredHeightWithDontChangeToggle);

    warningLabel.setBounds(left, psgFrequencyChooser.getBottom() + margins, width, labelsHeight);
    versionLabel.setBounds(left, getButtonsY() - labelsHeight - margins, width, labelsHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(sourceLabelsPrefixLabel);
    addComponentToModalDialog(sourceLabelsPrefixEditor);
    addComponentToModalDialog(psgFrequencyChooser);
    addComponentToModalDialog(warningLabel, false);
    addComponentToModalDialog(versionLabel);

    const auto sourceLabelsPrefix = PreferencesManager::getInstance().getSourceLabelsPrefix();
    sourceLabelsPrefixEditor.setText(sourceLabelsPrefix, false);
}

void ExportFapDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportFapDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::fap, FileExtensions::fapExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto song = songController.getSong();
    const auto baseLabel = sourceLabelsPrefixEditor.getText();

    // Stores the base label.
    const auto& preferences = PreferencesManager::getInstance();
    preferences.setSourceLabelsPrefix(baseLabel);
    // Stores the selected frequency.
    const auto forcedPsgFrequencyHz = psgFrequencyChooser.getSelectedFrequencyHz();
    preferences.storeFapOutputFrequencyHz(forcedPsgFrequencyHz);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto fapExporterTask = std::make_unique<FapExporter>(song, subsongId, sourceConfiguration, baseLabel, forcedPsgFrequencyHz);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<FapResult>>>(
        juce::translate("Exporting"), juce::translate("Please wait..."),*this, std::move(fapExporterTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportFapDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<FapResult> result) noexcept
{
    backgroundTask->clear();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (should only happen if digidrums).
    if ((taskOutputState != TaskOutputState::finished || (result == nullptr))) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to FAP! (Did you use digidrums? If no, please report this.)"),
                                                            [&] { onDialogExit(false); });
        return;
    }

    dialog = manageResultAndShowDialog(*result, fileToSaveTo, sourceConfiguration, [&] (const bool mustExit) {
        onDialogExit(mustExit);
    });
}

// ======================================================

void ExportFapDialog::onSubsongChanged(const Id& selectedSubsongId) noexcept
{
    const auto showWarning = (songController.getPsgCount(selectedSubsongId) != 1);
    warningLabel.setVisible(showWarning);
}

void ExportFapDialog::onDialogExit(const bool exit) noexcept
{
    dialog.reset();

    if (exit) {
        onCancelButtonClicked();
    }
}

std::unique_ptr<ModalDialog> ExportFapDialog::manageResultAndShowDialog(const FapResult& fapResult, const juce::File& fileToSaveTo,
    const SourceGeneratorConfiguration& sourceConfiguration, const std::function<void(bool mustExit)>& onDialogExit) noexcept
{
    // Ok, not on a background thread... Oh, well. The task should have been given a FileOutputStream.
    const auto& [fapData, constantSourceMemoryBlock, bufferSize, playTimeInNops, registerCountToPlay, isR12Constant] = fapResult;
    const auto binarySize = static_cast<int>(fapResult.fapData.getSize());
    auto success = (bufferSize > 0) && FileUtil::saveMemoryBlockToFile(fileToSaveTo, fapData);

    // Also saves the constants source file.
    const auto constantFileToSaveTo = FileUtil::buildFileWithSuffixAndExtension(fileToSaveTo, FapExporter::constantSourceFileSuffix,
        sourceConfiguration.getSourceFileExtension());
    success = success && FileUtil::saveMemoryBlockToFile(constantFileToSaveTo, constantSourceMemoryBlock);

    if (success) {
        return SuccessOrErrorDialog::buildForSuccess("Export to FAP finished!"
                                                       "\n\nBinary size: " + juce::String(binarySize) + " (#" + NumberUtil::toHexFourDigits(binarySize) + ")."
                                                       "\nBuffer size: " + juce::String(bufferSize) + " (#" + NumberUtil::toHexFourDigits(bufferSize) + ")."
                                                       "\nCPU in nops: " + juce::String(playTimeInNops) +
                                                       ".\nRegister count: " + juce::String(registerCountToPlay) +
                                                       ".\nR12 constant? " + StringUtil::boolToYesOrNo(isR12Constant) + ".",
            [=] { onDialogExit(true); }, SuccessOrErrorDialog::defaultWidth, 195);
    }
    return SuccessOrErrorDialog::buildForError("Unable to save the FAP file.", [&] { onDialogExit(false); });
}

}   // namespace arkostracker
