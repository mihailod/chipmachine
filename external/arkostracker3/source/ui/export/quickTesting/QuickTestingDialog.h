#pragma once

#include "../../../export/PlayerType.h"
#include "../../../song/Song.h"
#include "../../components/GroupWithToggles.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTaskWithProgress.h"

namespace arkostracker
{

class MainController;

/** Dialog to export to DSK or SNA, in AKG, AKY, etc. according to what is possible. */
class QuickTestingDialog : public ModalDialog,
                           public BackgroundTaskListener<std::unique_ptr<bool>>

{
public:
    /**
     * @param mainController the Main Controller.
     * @param listener to be aware when to close this dialog.
     */
    QuickTestingDialog(const MainController& mainController, std::function<void()> listener) noexcept;

    // BackgroundTaskListener method implementations.
    // ======================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;

private:
    enum class SelectedFormat : uint8_t
    {
        akg,
        aky,
        akySid,
        modCpcOld,
    };

    static int previouslySelectedTargetToIndex;
    static SelectedFormat previouslySelectedFormat;

    static constexpr auto targetIndexDsk = 0;
    static constexpr auto targetIndexSna = targetIndexDsk + 1;

    void onExportButtonClicked() noexcept;
    void onCancelButtonClicked() const noexcept;

    void determineFeaturesAndUpdateUi() noexcept;

    const MainController& mainController;
    std::function<void()> listener;

    GroupWithToggles exportTargetGroup;
    juce::GroupComponent exportFormatGroup;
    juce::ToggleButton akgToggleButton;
    juce::Label akgWarningLabel;
    juce::ToggleButton akyToggleButton;
    juce::Label akyWarningLabel;
    juce::ToggleButton akySidToggleButton;
    juce::Label akySidWarningLabel;
    juce::ToggleButton akyModCpcOldToggleButton;
    juce::Label akyModCpcOldWarningLabel;

    std::shared_ptr<Song> song;     // The stripped Song.

    OptionalValue<PlayerType> akyDeterminedPlayerType;     // What player to use if AKY?

    std::unique_ptr<BackgroundTaskWithProgress<std::unique_ptr<bool>>> backgroundTask;
    std::unique_ptr<ModalDialog> alertDialog;
};

}       // namespace arkostracker
