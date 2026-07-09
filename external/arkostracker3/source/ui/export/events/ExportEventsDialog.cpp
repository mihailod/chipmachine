#include "ExportEventsDialog.h"

#include "EventsExportType.h"
#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/events/EventsExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

ExportEventsDialog::ExportEventsDialog(MainController& pMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Export events"), 400, 510,
                    [&] { onExportButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        songController(pMainController.getSongController()),
        preferences(PreferencesManager::getInstance()),
        listener(std::move(pListener)),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& nameAndId) { onSelectedSubsongChanged(nameAndId.first); }),
        eventTypesComboBox(),
        exportAs(false, juce::String(), false),
        backgroundTask(),
        fileToSaveTo(),
        exportAsResult(),
        failureDialog(),
        saveSourceOrBinary()
{
    setOkButtonText(juce::translate("Export"));
    setOkButtonWidth(70);
    setCancelButtonText(juce::translate("Close"));

    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    eventTypesComboBox.setBounds(left, subsongChooser.getBottom() + margins * 2, width, labelsHeight);
    exportAs.setBounds(left, eventTypesComboBox.getBottom() + margins * 2, width, ExportAs::desiredHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(eventTypesComboBox);
    addComponentToModalDialog(exportAs);

    eventTypesComboBox.addItem(juce::translate("Export all events"), static_cast<int>(EventsExportType::exportAllEvents));
    eventTypesComboBox.addItem(juce::translate("Export only sample-related events"), static_cast<int>(EventsExportType::exportOnlySamples));
    eventTypesComboBox.addItem(juce::translate("Export only non-sample-related events"), static_cast<int>(EventsExportType::exportOnlyEvents));
    eventTypesComboBox.setSelectedId(static_cast<int>(PreferencesManager::getInstance().getEventsExportType()));
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportEventsDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to export events! Please report this."),
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
                                                              juce::translate("Export to events finished successfully!"),
                                                              juce::translate("Export to events failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) {
                                                                  onSaveSourceDialogOkClicked(success);
                                                              });
    saveSourceOrBinary->perform();
}

// ======================================================


void ExportEventsDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportEventsDialog::onSelectedSubsongChanged(const juce::String& subsongName) noexcept
{
    exportAs.onSubsongChanged(subsongName);
}

void ExportEventsDialog::onSaveSourceDialogOkClicked(bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportEventsDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

void ExportEventsDialog::onExportButtonClicked() noexcept
{
    // Saves the data from the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAs.storeChanges());

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::events, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const ExportConfiguration exportConfiguration(sourceConfiguration, { subsongId }, baseLabel, address);

    // Saves the export type.
    const auto selectedEventType = static_cast<EventsExportType>(eventTypesComboBox.getSelectedId());
    PreferencesManager::getInstance().storeEventsExportType(selectedEventType);

    // What events to export?
    std::set<EventsExporter::Type> typesToExport;
    switch (selectedEventType) {
        case EventsExportType::exportOnlySamples:
            typesToExport.emplace(EventsExporter::Type::samples);
            break;
        case EventsExportType::exportOnlyEvents:
            typesToExport.emplace(EventsExporter::Type::events);
            break;
        default:
            jassertfalse;
        case EventsExportType::exportAllEvents:
            typesToExport.emplace(EventsExporter::Type::samples);
            typesToExport.emplace(EventsExporter::Type::events);
            break;
    }

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporter = std::make_unique<EventsExporter>(song, typesToExport, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(exporter));
    backgroundTask->performTask();
}

}   // namespace arkostracker
