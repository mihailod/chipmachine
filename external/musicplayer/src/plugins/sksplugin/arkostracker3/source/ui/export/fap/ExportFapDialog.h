#pragma once

#include "../../../export/fap/FapExporter.h"
#include "../../components/EditText.h"
#include "../../components/PsgFrequencyChooser.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker 
{

class MainController;

/** Dialog to export to FAP. */
class ExportFapDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<FapResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportFapDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<FapResult> result) noexcept override;

    /**
     * Shared method once the result is known. Shows a dialog indicating the result or failure.
     * @param fapResult the FAP result.
     * @param fileToSaveTo the file to save the FAP binary to.
     * @param sourceConfiguration the source configuration.
     * @param onDialogExit called after the dialog is dismissed. True to exit this dialog.
     * @return the dialog to display.
     */
    static std::unique_ptr<ModalDialog> manageResultAndShowDialog(const FapResult& fapResult, const juce::File& fileToSaveTo,
                                                                  const SourceGeneratorConfiguration& sourceConfiguration,
                                                                  const std::function<void(bool mustExit)>& onDialogExit) noexcept;

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

    /**
     * Called when the user selected another Subsong.
     * @param selectedSubsongId the ID of the Subsong.
     */
    void onSubsongChanged(const Id& selectedSubsongId) noexcept;

    SubsongChooser subsongChooser;
    juce::Label sourceLabelsPrefixLabel;                    // Visible only for Source.
    EditText sourceLabelsPrefixEditor;

    PsgFrequencyChooser psgFrequencyChooser;

    juce::Label warningLabel;
    juce::Label versionLabel;

    SourceGeneratorConfiguration sourceConfiguration;
    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<FapResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker
