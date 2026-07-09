#pragma once

#include "../../../business/song/tool/sfx/FirstNotePerInstrumentFinder.h"
#include "../../../export/SongExportResult.h"
#include "../../components/BlankComponent.h"
#include "../../components/GroupWithViewport.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportAs.h"
#include "../common/task/SaveSourceOrBinaryDialog.h"

namespace arkostracker
{

class MainController;
class SongController;
class PlayerController;

/** Shows a Dialog to export to SFXs. */
class ExportSfxsDialog final : public ModalDialog,
                               public BackgroundTaskListener<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportSfxsDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SongExportResult> result) noexcept override;

private:
    /** Item displayed for each SFX. */
    class SfxItem
    {
    public:
        SfxItem(std::unique_ptr<juce::Label> instrumentLabel,
                std::unique_ptr<juce::TextButton> noteButton, std::unique_ptr<juce::ToggleButton> exportedToggle,
                std::unique_ptr<juce::Label> outputInstrumentIndexLabel);

        const std::unique_ptr<juce::Label>& getInstrumentLabel() const noexcept;
        const std::unique_ptr<juce::TextButton>& getNoteButton() const noexcept;
        const std::unique_ptr<juce::ToggleButton>& getExportedToggle() const noexcept;
        const std::unique_ptr<juce::Label>& getOutputInstrumentIndexLabel() const noexcept;

    private:
        std::unique_ptr<juce::Label> instrumentLabel;
        std::unique_ptr<juce::TextButton> noteButton;
        std::unique_ptr<juce::ToggleButton> exportedToggle;
        std::unique_ptr<juce::Label> outputInstrumentIndexLabel;
    };

    /** Called when the Save button is clicked. */
    void onSaveButtonClicked() noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /** Creates the sound effect items in the sfx group, without relevant data. */
    void createSoundEffectRawItems() noexcept;

    /** Exits this dialog by calling the listener. */
    void exit() const noexcept;

    /**
     * Called when an Exported toggle is clicked. The UI should be refreshed.
     * @param instrumentIndex the instrument index related to the toggle.
     */
    void onExportedToggled(int instrumentIndex) noexcept;

    /**
     * Called when the Play Button is clicked.
     * @param instrumentIndex the instrument index to play.
     */
    void onPlayButtonClicked(int instrumentIndex) noexcept;

    /** Updates the already ready UI from the internal data. */
    void updateUiFromData() noexcept;

    /** Stop is clicked. Stops the sounds. */
    void onStopButtonClicked() const noexcept;

    /** Exports the sound effects. */
    void onExportButtonClicked() noexcept;

    /** The user exited the Failure Dialog. */
    void onFailureDialogExit() noexcept;

    /**
     * Called when OK is clicked on final Save Dialog.
     * @param success true if the export was successful.
     */
    void onSaveSourceDialogOkClicked(bool success) noexcept;

    /** Stores the SFX data from the internal data, into the Song. */
    void storeSfxData() noexcept;

    PlayerController& playerController;
    SongController& songController;
    std::function<void()> listener;
    std::map<int, FirstNotePerInstrumentFinder::ResultItem> instrumentToFirstNote;
    std::vector<SfxItem> sfxViewedItems;                      // The UI for each item. The data matches the list above.

    juce::TextButton exportButton;
    GroupWithViewport sfxGroup;
    ExportAs exportAsDialog;

    juce::Label instrumentNameLegend;
    juce::Label noteLegend;
    juce::Label exportedLegend;
    juce::Label outputInstrumentIndexLegend;
    BlankComponent separator;

    juce::Label testParametersLabel;
    juce::ComboBox volumeComboBox;
    juce::TextButton stopButton;

    std::unique_ptr<ExportAs::Result> exportAsResult;
    juce::File fileToSaveTo;
    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>> backgroundTask;
    std::unique_ptr<ModalDialog> failureDialog;
    std::unique_ptr<SaveSourceOrBinaryDialog> saveSourceOrBinary;
};

}   // namespace arkostracker
