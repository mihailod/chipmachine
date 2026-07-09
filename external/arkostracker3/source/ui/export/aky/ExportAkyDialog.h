#pragma once

#include "../../../export/aky/AkyExporter.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Dialog to export to AKY. */
class ExportAkyDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportAkyDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

protected:
    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:

    /**
     * Called when the selected Subsong has changed.
     * @param subsongName the name of the selected Subsong.
     * @param subsongId the ID of the selected Subsong.
     */
    void onSelectedSubsongChanged(const juce::String& subsongName, const Id& subsongId) noexcept;

    /**
     * Called when OK is clicked on final Save Dialog.
     * @param success true if the export was successful.
     */
    void onSaveSourceDialogOkClicked(bool success) noexcept;

    /** The user exited the Failure Dialog. */
    void onFailureDialogExit() noexcept;

    PreferencesManager& preferences;

    SubsongChooser subsongChooser;
    ExportAs exportAs;

    juce::ToggleButton encodeAllAddressesAsRelativeToggle;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ExportAs::Result> exportAsResult;
    std::unique_ptr<ModalDialog> failureDialog;

    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;

    juce::GroupComponent sidPlayerFrequencyGroup;
    juce::Label sidPlayerFrequencyText;
};

}   // namespace arkostracker
