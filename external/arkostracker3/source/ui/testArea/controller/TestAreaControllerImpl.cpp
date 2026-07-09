#include "TestAreaControllerImpl.h"

#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../view/EditChannelsDialog.h"
#include "../view/TestAreaViewImpl.h"
#include "../view/TestAreaViewNoOp.h"

namespace arkostracker 
{

TestAreaControllerImpl::TestAreaControllerImpl(MainController& pMainController) noexcept :
        mainController(pMainController),
        playerController(pMainController.getPlayerController()),
        useSelectedArp(playerController.isSelectedArpUsedWhenPlayingNote()),
        useSelectedPitch(playerController.isSelectedPitchUsedWhenPlayingNote()),
        useMono(playerController.isUsingMonophony()),
        noteToShow(),
        pendingNoteToShow(),
        usedArpeggioIndex(),
        usedPitchIndex(),
        view(std::make_unique<TestAreaViewNoOp>()),         // At first, no real view is used.
        editChannelsDialog()
{
    playerController.getSongPlayerObservers().addObserver(this);
    mainController.observers().getSelectedExpressionIndexObservers(true).addObserver(this);
    mainController.observers().getSelectedExpressionIndexObservers(false).addObserver(this);
}

TestAreaControllerImpl::~TestAreaControllerImpl()
{
    mainController.observers().getSelectedExpressionIndexObservers(true).removeObserver(this);
    mainController.observers().getSelectedExpressionIndexObservers(false).removeObserver(this);
    playerController.getSongPlayerObservers().removeObserver(this);
}


// TestAreaController method implementations.
// =============================================

void TestAreaControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    // We can now create a "real" View.
    view = std::make_unique<TestAreaViewImpl>(*this, buildDisplayedData());
    parentView.addAndMakeVisible(*view);
}

void TestAreaControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    view->setBounds(startX, startY, newWidth, newHeight);
}

void TestAreaControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
    /* Nothing to do. */
}

void TestAreaControllerImpl::onParentViewDeleted() noexcept
{
    // Deletes the View, using a no-op View.
    view = std::make_unique<TestAreaViewNoOp>();
}

void TestAreaControllerImpl::onUserWantsToToggleUseSelectedArp() noexcept
{
    useSelectedArp = playerController.toggleIsSelectedArpUsed();
    updateView();
}

void TestAreaControllerImpl::onUserWantsToToggleUseSelectedPitch() noexcept
{
    useSelectedPitch = playerController.toggleIsSelectedPitchUsed();
    updateView();
}

void TestAreaControllerImpl::onUserWantsToToggleMonophony() noexcept
{
    useMono = playerController.toggleIsUsingMonophony();
    updateView();
}

void TestAreaControllerImpl::onUserWantsToPlaySound() noexcept
{
    playerController.play({ }, false, { });
}

void TestAreaControllerImpl::onUserWantsToPlayNote(const int noteNumber) noexcept
{
    playerController.play(noteNumber, false, { });
}

void TestAreaControllerImpl::onUserWantsToEditChannels() noexcept
{
    // Gets the information to display.
    const auto& songController = mainController.getSongController();
    const auto channelCount = songController.getChannelCount();

    const auto monophonicChannel = playerController.getMonophonicChannel();
    const auto polyphonicChannels = playerController.getPolyphonicChannels();

    const auto monophonicUsesCursorPosition = playerController.isMonophonicUsesCursorPosition();

    jassert(editChannelsDialog == nullptr);         // Already present?
    editChannelsDialog = std::make_unique<EditChannelsDialog>(channelCount, monophonicUsesCursorPosition, monophonicChannel, polyphonicChannels,
                                                              [&](const bool pMonophonicUsesCursorPosition, const int pMonophonicChannel,
                                                                  const std::set<int>& pPolyphonicChannels) {
                                                                  onEditChannelsOk(pMonophonicUsesCursorPosition, pMonophonicChannel, pPolyphonicChannels);
                                                              }, [&] { onEditChannelsCancelled(); });
}


// SongPlayerObserver method implementations.
// =============================================

void TestAreaControllerImpl::onNewTestNote(const int newNote) noexcept
{
    noteToShow = newNote;
    pendingNoteToShow = newNote;
    updateView();
}


// SelectedExpressionIndexObserver method implementations.
// ==========================================================

void TestAreaControllerImpl::onSelectedExpressionChanged(const bool isArpeggio, const OptionalId& selectedExpressionId, bool /*forceSingleSelection*/)
{
    auto& usedIndex = isArpeggio ? usedArpeggioIndex : usedPitchIndex;
    auto newIndex = 0;
    // Finds the index from the expression ID.
    if (selectedExpressionId.isPresent()) {
        const auto indexOptional = mainController.getSongController().getExpressionIndex(isArpeggio, selectedExpressionId.getValueRef());
        jassert(indexOptional.isPresent());
        if (indexOptional.isPresent()) {
            newIndex = indexOptional.getValue();
        }
    }
    usedIndex = newIndex;

    updateView();
}

// =============================================

TestAreaView::DisplayedData TestAreaControllerImpl::buildDisplayedData() const noexcept
{
    return { useSelectedArp, useSelectedPitch, useMono, noteToShow, pendingNoteToShow, usedArpeggioIndex, usedPitchIndex};
}

void TestAreaControllerImpl::updateView() noexcept
{
    const auto displayedData = buildDisplayedData();
    view->setDisplayedData(displayedData);

    pendingNoteToShow = OptionalInt();      // The note is consumed, removes it.
}

void TestAreaControllerImpl::onEditChannelsOk(const bool monophonyUsesCursorPosition, const int monophonicChannel, const std::set<int>& polyphonicChannels) noexcept
{
    playerController.setMonophonicUsesCursorPosition(monophonyUsesCursorPosition);
    playerController.setMonophonicChannelToUse(monophonicChannel);
    playerController.setPolyphonicChannelsToUse(polyphonicChannels);

    editChannelsDialog.reset();     // Must be done after the dialog data is retrieved.
}

void TestAreaControllerImpl::onEditChannelsCancelled() noexcept
{
    editChannelsDialog.reset();
}

void TestAreaControllerImpl::getKeyboardFocus() noexcept
{
    view->grabKeyboardFocus();
}

}   // namespace arkostracker
