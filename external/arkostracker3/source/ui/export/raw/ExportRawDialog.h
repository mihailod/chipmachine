#pragma once

#include <functional>
#include <juce_core/juce_core.h>

#include "../../../export/SongExportResult.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/ExportDialog.h"
#include "../common/SampleExport.h"
#include "../common/SubsongChooser.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker
{

class PreferencesManager;
class MainController;
class SongController;

/** Dialog to export to RAW. */
class ExportRawDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportRawDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
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
    ExportAs exportAs;
    SampleExport sampleExport;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ExportAs::Result> exportAsResult;
    std::unique_ptr<ModalDialog> failureDialog;

    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;

    juce::ToggleButton encodeSongSubsongMetadataToggleButton;
    juce::ToggleButton encodeReferenceTablesToggleButton;
    juce::ToggleButton encodeSpeedTracksToggleButton;
    juce::ToggleButton encodeEventTracksToggleButton;
    juce::ToggleButton encodeInstrumentsToggleButton;
    juce::ToggleButton encodeArpeggiosToggleButton;
    juce::ToggleButton encodePitchesToggleButton;
    juce::ToggleButton encodeEffectsToggleButton;
    juce::ToggleButton encodeEmptyLinesAsRleToggleButton;
    juce::ToggleButton encodeTranspositionsInLinkerToggleButton;
    juce::ToggleButton encodeHeightsInLinkerToggleButton;
    juce::Label pitchTrackRatioLabel;                                     // Label of the Pitch Ratio Ratio.
    EditText pitchTrackRatioTextEditor;                                   // Text Editor of the Pitch Ratio Ratio.
};

}   // namespace arkostracker
