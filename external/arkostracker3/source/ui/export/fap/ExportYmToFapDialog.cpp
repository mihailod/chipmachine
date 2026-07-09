#include "ExportYmToFapDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/ExportConfiguration.h"
#include "../../../export/fap/FapExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/MemoryBlockUtil.h"
#include "../../ProjectInfo.h"          // Generated file in the build folder.
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "ExportFapDialog.h"

namespace arkostracker
{

ExportYmToFapDialog::ExportYmToFapDialog(const MainController& pMainController, std::function<void()> pListener) noexcept:
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export a YM to FAP"), 400, 520),
        filePathChooser(juce::translate("Path to YM (drag'n'drop possible)"), juce::translate("Browse for YM"), FileExtensions::ymExtensionWithWildcard),
        sourceLabelsPrefixLabel(juce::String(), juce::translate("Source constant label prefix (may be empty):")),
        sourceLabelsPrefixEditor(),
        psgFrequencyChooser(true, juce::translate("PSG frequency")),
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

    filePathChooser.setBounds(left, top, width, FilePathChooser::desiredHeight);

    sourceLabelsPrefixLabel.setBounds(left, filePathChooser.getBottom() + margins, width, labelsHeight);
    sourceLabelsPrefixEditor.setBounds(left, sourceLabelsPrefixLabel.getBottom(), width, labelsHeight);

    const auto forcedPsgFrequencyHz = PreferencesManager::getInstance().getFapOutputFrequencyHz();
    psgFrequencyChooser.setFrequencyHz(forcedPsgFrequencyHz);
    psgFrequencyChooser.setBounds(left, sourceLabelsPrefixEditor.getBottom() + (margins * 2), width, PsgFrequencyChooser::desiredHeightWithDontChangeToggle);

    versionLabel.setBounds(left, getButtonsY() - labelsHeight - margins, width, labelsHeight);

    addComponentToModalDialog(filePathChooser);
    addComponentToModalDialog(sourceLabelsPrefixLabel);
    addComponentToModalDialog(sourceLabelsPrefixEditor);
    addComponentToModalDialog(psgFrequencyChooser);
    addComponentToModalDialog(versionLabel);

    const auto sourceLabelsPrefix = PreferencesManager::getInstance().getSourceLabelsPrefix();
    sourceLabelsPrefixEditor.setText(sourceLabelsPrefix, false);
}

void ExportYmToFapDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportYmToFapDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::fap, FileExtensions::fapExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    // Stores the selected frequency.
    const auto forcedPsgFrequencyHz = psgFrequencyChooser.getSelectedFrequencyHz();
    PreferencesManager::getInstance().storeFapOutputFrequencyHz(forcedPsgFrequencyHz);

    const auto song = songController.getSong();
    const auto baseLabel = sourceLabelsPrefixEditor.getText();

    // Stores the base label.
    PreferencesManager::getInstance().setSourceLabelsPrefix(baseLabel);

    // Loads the YM. It CAN be zipped! Should be asynchronous... Oh, well.
    const auto ymFile = filePathChooser.getSelectedFile();
    const auto ymMemoryBlock = MemoryBlockUtil::fromFile(ymFile);
    if (ymMemoryBlock == nullptr) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to load the YM!"),
                                                    [&] { onDialogExit(false); });
        return;
    }

    // Creates the exporter, and the Task to perform it asynchronously.
    auto fapExporterTask = std::make_unique<FapExporter>(*ymMemoryBlock, sourceConfiguration, baseLabel, forcedPsgFrequencyHz);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<FapResult>>>(
        juce::translate("Exporting"), juce::translate("Please wait..."),
        *this, std::move(fapExporterTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportYmToFapDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<FapResult> result) noexcept
{
    backgroundTask->clear();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (should only happen if digidrums).
    if ((result == nullptr) || (taskOutputState != TaskOutputState::finished)) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to load the YM, or convert it to FAP! (Did you use digidrums? They are forbidden by FAP)"),
                                                            [&] { onDialogExit(false); });
        return;
    }

    dialog = ExportFapDialog::manageResultAndShowDialog(*result, fileToSaveTo, sourceConfiguration, [&] (const bool mustExit) {
        onDialogExit(mustExit);
    });
}


// ======================================================

void ExportYmToFapDialog::onDialogExit(const bool exit) noexcept
{
    dialog.reset();

    if (exit) {
        onCancelButtonClicked();
    }
}

}   // namespace arkostracker
