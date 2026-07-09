#pragma once

#include "../../../song/Song.h"
#include "../../components/EditText.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"

namespace arkostracker
{

class MainController;
class SongController;

/** Shows a Dialog to export to WAV. */
class ExportInstrumentToWavDialog final : public ModalDialog,
                                          public BackgroundTaskListener<std::unique_ptr<std::vector<juce::File>>>,
                                          public juce::ComboBox::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportInstrumentToWavDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<std::vector<juce::File>> result) noexcept override;

    // juce::ComboBox::Listener method implementations.
    // ======================================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

private:
    static constexpr auto offsetPsgId = 1;
    static constexpr auto offsetRangeId = 1;

    /** Called when the Export button is clicked. */
    void onExportButtonClicked() noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /** Closes the possible shown Modal Dialog. */
    void closeShownDialog() const noexcept;
    /** Exits this dialog by calling the listener. */
    void exit() const noexcept;

    /** Fills the instrument combo box, clearing it first. */
    void fillInstrumentComboBox() noexcept;
    /** Fills the PSG combo box, clearing it first. */
    void fillPsgComboBox() noexcept;
    /** Fills the Replay frequency combo box, clearing it first. */
    void fillReplayFrequencyComboBox() noexcept;
    /** Fills the range combo boxes, clearing them first. */
    void fillRangeComboBoxes() noexcept;
    /** Fills one range combo box, clearing it first. */
    static void fillRangeComboBox(const juce::String& prefix, juce::ComboBox& comboBox) noexcept;

    std::shared_ptr<Song> song;
    std::function<void()> listener;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<std::vector<juce::File>>>> progressTask;

    std::unique_ptr<ModalDialog> dialog;

    juce::File folderToSaveTo;

    juce::Label instrumentToExportLabel;
    juce::ComboBox instrumentToExportComboBox;
    juce::Label psgLabel;
    juce::ComboBox psgComboBox;
    juce::Label replayFrequencyHzLabel;
    juce::ComboBox replayFrequencyHzComboBox;

    juce::Label baseFilenameLabel;
    EditText baseFilenameEditText;

    juce::Label noteRangeLabel;
    juce::ComboBox fromNoteComboBox;
    juce::ComboBox toNoteComboBox;

    juce::Label minimumDurationSecondsLabel;
    EditText minimumDurationSecondsEditText;

    std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> baseFilenameRestriction;
    juce::TextEditor::LengthAndCharacterRestriction minimumDurationSecondsRestriction;

    std::vector<Psg> psgs;
    std::vector<float> replayFrequenciesHz;

    OptionalId selectedInstrumentId;
};

}   // namespace arkostracker
