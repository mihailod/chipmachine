#pragma once

#include "../../components/GroupWithViewport.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"
#include "../common/ExportDialog.h"
#include "../common/SubsongChooser.h"

namespace arkostracker
{

class MainController;
class SongController;

/** Dialog to export to Midi. */
class ExportMidiDialog final : public ExportDialog,
                               public BackgroundTaskListener<std::unique_ptr<juce::MemoryBlock>>
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     */
    ExportMidiDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<juce::MemoryBlock> result) noexcept override;

    // ExportDialog method implementations.
    // ======================================================
    void onExportButtonClicked() noexcept override;
    void onCancelButtonClicked() const noexcept override;

private:
    static constexpr auto idDontExport = -1;
    static constexpr auto offsetIdNormalTracks = 1;
    static constexpr auto offsetDrumsId = 1000;
    static constexpr auto midiDrumsMinimumIndex = 35;
    static constexpr auto midiDrumsMaximumIndex = 81;

    /** Holds each instrument Component for one item. */
    class InstrumentItem final
    {
    public:
        InstrumentItem(
            const int pInstrumentIndex,
            std::unique_ptr<juce::Label> pName,
            std::unique_ptr<juce::ComboBox> pTarget) :
                instrumentIndex(pInstrumentIndex),
                name(std::move(pName)),
                target(std::move(pTarget))
        {
        }

        int instrumentIndex;
        std::unique_ptr<juce::Label> name;
        std::unique_ptr<juce::ComboBox> target;
    };

    /** The user exited a Dialog. */
    void onDialogExit() noexcept;

    /** Builds the content of the instrument group. */
    void buildInstrumentGroup() noexcept;

    static void fillTargetView(juce::ComboBox& comboBox, int selectedTrackIndex, int maximumTrackIndex) noexcept;

    SubsongChooser subsongChooser;
    GroupWithViewport instrumentGroup;
    juce::ToggleButton exportMarkersToggle;
    juce::Label drumChannelLabel;                           // Presents the drum channel.
    juce::Slider drumChannelSlider;                         // Slider for the drum channel.

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryBlock>>> backgroundTask;

    juce::File fileToSaveTo;
    std::unique_ptr<ModalDialog> dialog;

    std::vector<std::unique_ptr<InstrumentItem>> instrumentItems;
};

}   // namespace arkostracker
