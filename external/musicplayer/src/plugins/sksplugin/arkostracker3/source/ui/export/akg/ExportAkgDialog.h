#pragma once

#include "../../../export/SongExportResult.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/ExportDialog.h"
#include "../common/SampleExport.h"
#include "../common/SubsongsChooser.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker 
{

class PreferencesManager;
class MainController;
class SongController;

/** Dialog to export to AKG. */
class ExportAkgDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportAkgDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    /**
     * Called when OK is clicked on final Save Dialog.
     * @param success true if the export was successful.
     */
    void onSaveSourceDialogOkClicked(bool success) noexcept;

    /** The user exited the Failure Dialog. */
    void onFailureDialogExit() noexcept;

    PreferencesManager& preferences;

    SubsongsChooser subsongsChooser;
    ExportAs exportAsDialog;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;

    juce::File fileToSaveTo;
    SampleExport sampleExport;
    std::unique_ptr<ExportAs::Result> exportAsResult;
    std::unique_ptr<ModalDialog> failureDialog;

    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;
};

}   // namespace arkostracker
