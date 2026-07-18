#pragma once

#include "../../../export/fap/FapExporter.h"
#include "../../components/EditText.h"
#include "../../components/FilePathChooser.h"
#include "../../components/PsgFrequencyChooser.h"
#include "../../utils/backgroundTask/BackgroundTask.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"

namespace arkostracker
{

class MainController;

/** Dialog to export a YM to FAP. */
class ExportYmToFapDialog final : public ExportDialog,
                                  public BackgroundTaskListener<std::unique_ptr<FapResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportYmToFapDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<FapResult> result) noexcept override;

protected:
    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    /**
     * The user exited a Dialog.
     * @param exit true to exit this page.
     */
    void onDialogExit(bool exit = false) noexcept;

    FilePathChooser filePathChooser;

    juce::Label sourceLabelsPrefixLabel;                    // Visible only for Source.
    EditText sourceLabelsPrefixEditor;

    PsgFrequencyChooser psgFrequencyChooser;

    juce::Label versionLabel;

    SourceGeneratorConfiguration sourceConfiguration;
    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<FapResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker
