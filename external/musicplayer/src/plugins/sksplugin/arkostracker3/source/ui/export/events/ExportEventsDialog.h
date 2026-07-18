#pragma once

#include <functional>
#include <juce_core/juce_core.h>

#include "../../../export/SongExportResult.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/SubsongChooser.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker
{

class PreferencesManager;
class MainController;
class SongController;

/** Dialog to export events. */
class ExportEventsDialog final : public ModalDialog,
                                 public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportEventsDialog(MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

private:
    /** Called when the Export button is clicked. */
    void onExportButtonClicked() noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /**
     * Called when the selected Subsong has changed.
     * @param subsongName the name of the selected Subsong.
     */
    void onSelectedSubsongChanged(const juce::String& subsongName) noexcept;

    /**
     * Called when OK is clicked on final Save Dialog.
     * @param success true if the export was successful.
     */
    void onSaveSourceDialogOkClicked(bool success) noexcept;

    /** The user exited the Failure Dialog. */
    void onFailureDialogExit() noexcept;

    SongController& songController;
    PreferencesManager& preferences;

    std::function<void()> listener;

    SubsongChooser subsongChooser;
    juce::ComboBox eventTypesComboBox;
    ExportAs exportAs;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ExportAs::Result> exportAsResult;
    std::unique_ptr<ModalDialog> failureDialog;

    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;
};

}   // namespace arkostracker
