#include "ExportRawLinearDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/events/EventsExporter.h"
#include "../../../export/raw/RawExporter.h"
#include "../../../export/rawLinear/RawLinearExport.h"
#include "../../../utils/NumberUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

ExportRawLinearDialog::ExportRawLinearDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to RAW Linear"), 500, 570),
        preferences(PreferencesManager::getInstance()),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& nameAndId) { onSelectedSubsongChanged(nameAndId.first); }),
        exportTypeGroup(juce::translate("Export..."), 3, false, true, 400,
            [](const int index) {
            switch (index) {
                default:
                    jassertfalse;
                case indexExportAll:
                    return juce::translate("Both PSG and sample notes");
                case indexExportPsgOnly:
                    return juce::translate("Only PSG notes");
                case indexExportSamplesOnly:
                    return juce::translate("Only sample notes");
            }
        }, [&](const int) {
            return false;   // Set below.
        }),
        exportAs(false, juce::String(), false),
        backgroundTask(),
        fileToSaveTo(),
        exportAsResult(),
        failureDialog(),
        saveSourceOrBinary()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    exportTypeGroup.setBounds(left, subsongChooser.getBottom() + margins, width, 106);
    exportAs.setBounds(left, exportTypeGroup.getBottom() + margins, width, ExportAs::desiredHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(exportTypeGroup);
    addComponentToModalDialog(exportAs);

    // Sets the export type in its group.
    const auto exportType = static_cast<int>(PreferencesManager::getInstance().getRawLinearExportType());
    exportTypeGroup.setSelected({ exportType });
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportRawLinearDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to export to RAW! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(),
                                                              fileToSaveTo, false, saveToBinary,
                                                              false, PlayerConfiguration(),
                                                              sourceConfiguration,
                                                              juce::translate("Export to RAW Linear finished successfully!"),
                                                              juce::translate("Export to RAW Linear failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) {
                                                                  onSaveSourceDialogOkClicked(success);
                                                              });
    saveSourceOrBinary->perform();
}

// ======================================================


void ExportRawLinearDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportRawLinearDialog::onSelectedSubsongChanged(const juce::String& subsongName) noexcept
{
    exportAs.onSubsongChanged(subsongName);
}

void ExportRawLinearDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportRawLinearDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

void ExportRawLinearDialog::onExportButtonClicked() noexcept
{
    // Saves the data from the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAs.storeChanges());

    const auto exportTypeIndex = *exportTypeGroup.getToggledIndexes().cbegin(); // Only one is selectable.
    const auto exportType = static_cast<RawLinearExportType>(exportTypeIndex);
    PreferencesManager::getInstance().storeRawLinearExportType(exportType);

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::raw, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const ExportConfiguration exportConfiguration(sourceConfiguration, { subsongId }, baseLabel, address);

    std::set<InstrumentType> instrumentTypes;
    switch (exportType) {
        default:
            jassertfalse;
        case RawLinearExportType::all:
            instrumentTypes = { InstrumentType::psgInstrument, InstrumentType::sampleInstrument };
            break;
        case RawLinearExportType::psgOnly:
            instrumentTypes = { InstrumentType::psgInstrument };
            break;
        case RawLinearExportType::samplesOnly:
            instrumentTypes = { InstrumentType::sampleInstrument };
            break;
    }

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporter = std::make_unique<RawLinearExporter>(song, exportConfiguration, instrumentTypes);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(exporter));
    backgroundTask->performTask();
}

}   // namespace arkostracker
