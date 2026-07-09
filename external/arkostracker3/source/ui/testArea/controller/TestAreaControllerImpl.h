#pragma once

#include <set>

#include "TestAreaController.h"
#include "../../../controllers/observers/SelectedExpressionIndexObserver.h"
#include "../../../controllers/observers/SongPlayerObserver.h"
#include "../../../utils/OptionalValue.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../view/TestAreaView.h"

namespace arkostracker 
{

class MainController;
class PlayerController;

/**
 * Implementation of the Test Area Controller.
 *
 * One important thing to know is that the "PC keyboard" is not managed here, but by the main application target, which is handy.
 * The Test Area Controller is only notified that a note has been played.
 */
class TestAreaControllerImpl final : public TestAreaController,
                                     public SongPlayerObserver,         // To change the note being played in the Test button.
                                     public SelectedExpressionIndexObserver
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit TestAreaControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~TestAreaControllerImpl() override;

    // TestAreaController method implementations.
    // =============================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;

    void onUserWantsToToggleUseSelectedArp() noexcept override;
    void onUserWantsToToggleUseSelectedPitch() noexcept override;

    void onUserWantsToPlaySound() noexcept override;
    void onUserWantsToPlayNote(int noteNumber) noexcept override;

    void onUserWantsToToggleMonophony() noexcept override;
    void onUserWantsToEditChannels() noexcept override;

    void getKeyboardFocus() noexcept override;

    // SongPlayerObserver method implementations.
    // =============================================
    void onNewTestNote(int newNote) noexcept override;

    // SelectedExpressionIndexObserver method implementations.
    // ==========================================================
    void onSelectedExpressionChanged(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelection) override;

private:
    /** Updates the View thanks to the current states. */
    void updateView() noexcept;

    /** @return the data to display, from the current states. */
    TestAreaView::DisplayedData buildDisplayedData() const noexcept;

    /**
     * Called when the Edit Channels Dialog is closed, with an OK.
     * @param monophonyUsesCursorPosition true if monophony uses the cursor position. False to use a specific channel.
     * @param monophonicChannel the selected monophonic specific channel.
     * @param polyphonicChannels the polyphonic channels.
     */
    void onEditChannelsOk(bool monophonyUsesCursorPosition, int monophonicChannel, const std::set<int>& polyphonicChannels) noexcept;
    /** Called when the Edit Channels Dialog is cancelled. */
    void onEditChannelsCancelled() noexcept;

    MainController& mainController;
    PlayerController& playerController;

    bool useSelectedArp;
    bool useSelectedPitch;
    bool useMono;
    OptionalInt noteToShow;             // Empty at first, to show "play!" in the Button.
    OptionalInt pendingNoteToShow;      // Set to empty when consumed. Useful for the keyboard view for example.
    int usedArpeggioIndex;
    int usedPitchIndex;

    std::unique_ptr<TestAreaView> view;

    std::unique_ptr<ModalDialog> editChannelsDialog;
};
}   // namespace arkostracker
