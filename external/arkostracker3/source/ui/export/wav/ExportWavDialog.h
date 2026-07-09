#pragma once

#include "../../components/SliderIncDec.h"
#include "../../setup/output/OutputMixGroup.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker
{

class MainController;
class SongController;

/** Shows a Dialog to export to WAV. */
class ExportWavDialog final : public ExportDialog,
                              public BackgroundTaskListener<std::unique_ptr<bool>>,
                              public SliderIncDec::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportWavDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;

    // SliderIncDec::Listener method implementations.
    // ======================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

protected:
    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    static int minimumFrequencyHz;

    // The export values are stored locally. No need for more!
    static double storedExportFrequencyHz;

    static StoredOutputMix storedOutputMix;
    static int storedAdditionalLoopCount;
    static bool storedExportChannelsIndividually;

    /** Closes the possible shown Modal Dialog. */
    void closeShownDialog() const noexcept;
    /** Exits this dialog by calling the listener. */
    void exit() const noexcept;

    /** Saves locally the selected data. */
    bool saveData() const noexcept;

    /** @return the typed frequency is valid, and a boolean indicating whether it is valid. */
    std::pair<bool, int> getFrequencyHz() const noexcept;

    /** Called when the export frequency in Hz has changed. */
    void onFrequencyEditTextChanged() noexcept;

    SubsongChooser subsongChooser;
    juce::Label exportFrequencyLabel;
    EditText exportFrequencyEditText;
    juce::Label stereoSeparationLabel;
    juce::Slider stereoSeparationSlider;
    OutputMixGroup outputMixGroup;
    juce::ToggleButton exportEachChannelIndividuallyToggle;
    SliderIncDec additionalLoopCountSlider;

    juce::TextEditor::LengthAndCharacterRestriction restrictionHz;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> progressTask;

    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker
