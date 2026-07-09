#pragma once

#include <functional>
#include <juce_core/juce_core.h>

#include "../../../export/SongExportResult.h"
#include "../../components/GroupWithToggles.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker
{

class PreferencesManager;
class MainController;
class SongController;

/** Dialog to export to RAW Linear. */
class ExportRawLinearDialog final : public ExportDialog,
                                    public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportRawLinearDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    static constexpr auto indexExportAll = 0;
    static constexpr auto indexExportPsgOnly = 1;
    static constexpr auto indexExportSamplesOnly = 2;

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

    PreferencesManager& preferences;

    SubsongChooser subsongChooser;
    GroupWithToggles exportTypeGroup;
    ExportAs exportAs;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ExportAs::Result> exportAsResult;
    std::unique_ptr<ModalDialog> failureDialog;

    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;
};

}   // namespace arkostracker
