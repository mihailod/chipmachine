#pragma once

#include "../../../song/Song.h"
#include "../../components/SliderIncDec.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTask.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"

namespace arkostracker
{

class MainController;

/** Dialog to export to DSK, in AKG, interruption. */
class QuickExportAkgDialog : public ModalDialog,
                             public SliderIncDec::Listener
{
public:
    /**
     * @param mainController the Main Controller.
     * @param listener to be aware when to close this dialog.
     */
    QuickExportAkgDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // SliderIncDec::Listener method implementations.
    // ======================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    static constexpr auto defaultPlayerAddress = 0x9600;
    /** To put the music/sfx at anchor positions. */
    static constexpr auto addressMask = 0xff80U;

    void onExportButtonClicked() noexcept;
    void onCancelButtonClicked() const noexcept;

    /** Shows the addresses and sizes in the UI. */
    void updateAndShowAddressesAndSize() noexcept;

    /** Called when the SFX button is clicked. */
    void onSfxButtonClicked() noexcept;

    /** Called to close the possible alert dialog. */
    void closeAlertDialog() noexcept;

    /** @return true if the sfxs are present. */
    bool areSfxPresent() const noexcept;

    std::shared_ptr<const Song> song;
    std::function<void()> listener;

    juce::Label topLabel;

    int sizePlayerWithoutSfx;
    int sizePlayerWithSfx;
    int playerAddress;
    int musicAddress;
    int musicSize;
    int sfxAddress;
    OptionalValue<int> sfxSize;     // Absent when not known.

    juce::Label startLabel;
    juce::Label sizeLabel;
    juce::Label endLabel;
    SliderIncDec playerAddressSlider;
    juce::Label playerSizeLabel;
    juce::Label playerEndLabel;

    juce::Label musicLabel;
    juce::Label musicAddressLabel;
    juce::Label musicSizeLabel;
    juce::Label musicEndLabel;

    juce::Label sfxLabel;
    juce::Label sfxAddressLabel;
    juce::Label sfxSizeLabel;
    juce::Label sfxEndLabel;
    juce::TextButton sfxButton;

    juce::Label errorLabel;

    bool isError;

    std::unique_ptr<ModalDialog> alertDialog;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryBlock>>> loadSfxTask;
    juce::File sfxFileToLoad;

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> exportTask;
    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> initTask;

    // =============================================

    class LoadSfxTaskListener : public BackgroundTaskListener<std::unique_ptr<juce::MemoryBlock>>,
                                public WithParent<QuickExportAkgDialog>
    {
    public:
        explicit LoadSfxTaskListener(QuickExportAkgDialog& parent) noexcept;

        void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<juce::MemoryBlock> result) noexcept override;
    };
    LoadSfxTaskListener loadSfxTaskListener;

    // =============================================

    class ExportTaskListener : public BackgroundTaskListener<std::unique_ptr<bool>>,
                               public WithParent<QuickExportAkgDialog>
    {
    public:
        explicit ExportTaskListener(QuickExportAkgDialog& parent) noexcept;

        void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;
    };
    ExportTaskListener exportTaskListener;

    // =============================================

    class InitTaskListener : public BackgroundTaskListener<std::unique_ptr<bool>>,
                             public WithParent<QuickExportAkgDialog>
    {
    public:
        explicit InitTaskListener(QuickExportAkgDialog& parent) noexcept;

        void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;
    };
    InitTaskListener initTaskListener;

    // =============================================

    class InitTask : public WithParent<QuickExportAkgDialog>,
                     public Task<std::unique_ptr<bool>>
    {
    public:
        explicit InitTask(QuickExportAkgDialog& parent, std::shared_ptr<const Song> song) noexcept;

        // Task method implementations.
        // ===================================================
        std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

    private:
        std::shared_ptr<const Song> song;
    };
};

}       // namespace arkostracker
