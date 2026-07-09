#include "MainToolbarControllerImpl.h"

#include "../../../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../../../business/song/tool/speed/BpmCalculator.h"
#include "../../../../controllers/MainController.h"
#include "../../../../controllers/PlayerController.h"
#include "../../../../song/subsong/SubsongConstants.h"
#include "../../../../ui/containerArranger/StorageListener.h"
#include "../../../../utils/NumberUtil.h"
#include "../view/MainToolbarViewImpl.h"
#include "../view/MainToolbarViewNoOp.h"

namespace arkostracker
{

const int MainToolbarControllerImpl::timerRefreshRateMs = 500;

MainToolbarControllerImpl::MainToolbarControllerImpl(MainController& pMainController) noexcept :
        mainController(pMainController),
        storageListener(mainController.getStorageListener()),
        playedLocation(mainController.getSongController().getSong()->getFirstSubsongId(), 0),
        showRemainingDuration(false),
        cachedSpeed(SubsongConstants::defaultSpeed),
        view(std::unique_ptr<MainToolbarViewNoOp>())          // For now, a no-op implementation.
{
    // Observes the played position for the timer.
    mainController.getPlayerController().getSongPlayerObservers().addObserver(this);
    mainController.observers().getGeneralDataObservers().addObserver(this);
    mainController.getSongController().getSubsongMetadataObservers().addObserver(this);
}

MainToolbarControllerImpl::~MainToolbarControllerImpl()
{
    mainController.getPlayerController().getSongPlayerObservers().removeObserver(this);
    mainController.observers().getGeneralDataObservers().removeObserver(this);
    mainController.getSongController().getSubsongMetadataObservers().removeObserver(this);
    stopTimer();            // Only a security.
}


// SongPlayerObserver method implementations.
// ==============================================

void MainToolbarControllerImpl::onPlayerNewLocations(const SongPlayerObserver::Locations& locations) noexcept
{
    // Note: no change test is performed: if a height changes but the player cursor doesn't change, this must change the frame count anyway.
    playedLocation = locations.playedLocation;
}

void MainToolbarControllerImpl::onNewSpeed(const int newSpeed) noexcept
{
    cachedSpeed = newSpeed;
    const auto bpm = calculateBpm(newSpeed);

    view->setSpeed(newSpeed, bpm);
}

void MainToolbarControllerImpl::onPlayerPlayEvent(const Event event) noexcept
{
    MainToolbarView::PlayButton playButton;         // NOLINT(*-init-variables)
    switch (event) {
        case Event::playFromStart:
            playButton = MainToolbarView::PlayButton::playFromStart;
            break;
        case Event::play:
            playButton = MainToolbarView::PlayButton::play;
            break;
        case Event::playPatternFromStart:
            playButton = MainToolbarView::PlayButton::playPatternFromStart;
            break;
        case Event::playLine: [[fallthrough]];
        case Event::playPatternContinue: [[fallthrough]];
        case Event::playPatternFromTopOrBlock:
            playButton = MainToolbarView::PlayButton::playPattern;
            break;
        case Event::stop: [[fallthrough]];
        default:
            playButton = MainToolbarView::PlayButton::stop;
            break;
    }

    view->setHighlightedPlayIcon(playButton);
}


// Timer method implementations.
// ====================================

void MainToolbarControllerImpl::timerCallback()
{
    getDataAndUpdateTimer();
}


// ParentViewLifeCycleAware method implementations.
// ===================================================

void MainToolbarControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    // Creates the View.
    const auto octaveToShow = mainController.getCurrentOctave();
    constexpr auto layoutIndex = 0;         // A bit of a simplification, but works, since it is the default value.
    const auto speedToShow = SubsongConstants::defaultSpeed;         // Idem.
    const auto bpmToShow = calculateBpm(speedToShow);

    view = std::make_unique<MainToolbarViewImpl>(*this, layoutIndex, octaveToShow, speedToShow, bpmToShow);
    parentView.addAndMakeVisible(*view);

    getDataAndUpdateTimer();        // Do it as soon as possible, else it won't be shown right from the start.
    startTimer(timerRefreshRateMs);

    view->setHighlightedPlayIcon(MainToolbarView::PlayButton::stop);
}

void MainToolbarControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    view->setBounds(startX, startY, newWidth, newHeight);
}

void MainToolbarControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
    // Nothing to do.
}

void MainToolbarControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to the no-op view.
    view = std::make_unique<MainToolbarViewNoOp>();
    stopTimer();
}


// MainToolbarController method implementations.
// ================================================

void MainToolbarControllerImpl::onTimerClicked() noexcept
{
    // Shows the total or remaining, and forces the refresh.
    showRemainingDuration = !showRemainingDuration;
    timerCallback();
}


// ===================================================

void MainToolbarControllerImpl::getDataAndUpdateTimer() const noexcept {
    const auto subsongId = playedLocation.getSubsongId();

    // Gets the replay frequency.
    auto& songController = mainController.getSongController();
    auto replayFrequency = PsgFrequency::defaultReplayFrequencyHz;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        replayFrequency = subsong.getReplayFrequencyHz();
    });

    // Gets the Song and calculates its length till the current position.
    const auto song = songController.getSong();
    const auto [frameCounter, frameUpToCounter, loopCounter] = FrameCounter::count(*song, subsongId, playedLocation);
    // Shows the remaining or the progress?

    const auto frameCountToShow = showRemainingDuration ? (frameCounter - frameUpToCounter) : frameUpToCounter;

    // Converts to minutes/seconds for display.
    const auto firstDisplayDuration = getDisplayedDuration(frameCountToShow, replayFrequency, showRemainingDuration);
    const auto totalDisplayDuration = getDisplayedDuration(frameCounter, replayFrequency, false);

    view->setTimerToDisplay(firstDisplayDuration + "/" + totalDisplayDuration);
}

juce::String MainToolbarControllerImpl::getDisplayedDuration(const int frameCount, const float replayFrequency, const bool showMinus) noexcept
{
    const auto durationSeconds = static_cast<int>((static_cast<float>(frameCount) / replayFrequency));
    const auto minutesAndSeconds = NumberUtil::toMinutesAndSeconds(durationSeconds);
    return NumberUtil::getDisplayMinutesAndSeconds(minutesAndSeconds.first, minutesAndSeconds.second, showMinus);
}

void MainToolbarControllerImpl::onUserWantsToRestoreArrangement(const int arrangementNumber)
{
    storageListener.onRestoreArrangement(arrangementNumber);
}

void MainToolbarControllerImpl::onUserWantsToPlaySongFromStart() noexcept
{
    perform(CommandIds::playSongFromStart);
}

void MainToolbarControllerImpl::onUserWantsToPlaySong() noexcept
{
    perform(CommandIds::playSongContinue);
}

void MainToolbarControllerImpl::onUserWantsToPlayPatternFromStart() noexcept
{
    perform(CommandIds::playPatternFromStart);
}

void MainToolbarControllerImpl::onUserWantsToPlayPattern() noexcept
{
    perform(CommandIds::playPatternFromCursorOrBlock);
}

void MainToolbarControllerImpl::onUserWantsToStopPlay() noexcept
{
    perform(CommandIds::stop);
}

void MainToolbarControllerImpl::onUserWantsToSetOctave(const int desiredOctave) noexcept
{
    mainController.setOctave(desiredOctave);
}

void MainToolbarControllerImpl::perform(const CommandIds commandId) const noexcept
{
    mainController.getCommandManager().invokeDirectly(static_cast<int>(commandId), true);
}

void MainToolbarControllerImpl::onUserWantsToStartStopSerial() noexcept
{
    mainController.onUserWantsToStartStopSerial();
}

int MainToolbarControllerImpl::calculateBpm(const int speed) const noexcept
{
    auto bpm = 0;
    mainController.getSongController().performOnConstSubsong(playedLocation.getSubsongId(), [&](const Subsong& subsong) {
        const auto metadata = subsong.getMetadata();
        const auto rowCountPerBeat = metadata.getHighlightSpacing();
        const auto replayFrequencyHz = metadata.getReplayFrequencyHz();

        bpm = BpmCalculator::calculateBpm(rowCountPerBeat, speed, replayFrequencyHz);
    });

    return bpm;
}


// GeneralDataObserver method implementations.
// =============================================

void MainToolbarControllerImpl::onGeneralDataChanged(const unsigned int what)
{
    // Octave change?
    if (NumberUtil::isBitPresent(what,  GeneralDataObserver::What::octave)) {
        const auto lOctave = mainController.getCurrentOctave();
        view->setOctave(lOctave);
        return;
    }

    // New layout?
    if (NumberUtil::isBitPresent(what, GeneralDataObserver::What::layoutRestore)) {
        // What is the current layout index? Sets it to the view.
        const auto arrangementNumber = storageListener.getCurrentArrangementNumber();
        view->setCurrentArrangementNumber(arrangementNumber);
    }
}

void MainToolbarControllerImpl::onSerialCommunicationEvent(const SerialController::CommunicationState state, const SerialAccess::SerialOperationResult reason)
{
    bool iconConnected;         // NOLINT(*-init-variables)
    switch (state) {
        default:
        case SerialController::CommunicationState::stopped:
        case SerialController::CommunicationState::starting:
        case SerialController::CommunicationState::stopping:
            iconConnected = false;
            break;
        case SerialController::CommunicationState::started:
            iconConnected = true;
            break;
    }

    const auto iconEnabled = ((state != SerialController::CommunicationState::starting) && (state != SerialController::CommunicationState::stopping));

    view->setSerialIconState(iconConnected, iconEnabled);

    // Shows a possible error message.
    juce::String message;
    auto showSetupPanel = false;
    switch (reason) {
        case SerialAccess::SerialOperationResult::channelCouldNotBeOpened:
            message = juce::translate("The port could not be opened. Please check your hardware in the setup.");
            showSetupPanel = true;
            break;
        case SerialAccess::SerialOperationResult::portNameEmpty:
            message = juce::translate("No port is set up. Please check your hardware in the setup.");
            showSetupPanel = true;
            break;
        case SerialAccess::SerialOperationResult::writeFailed:
            message = juce::translate("Error while sending data to the port!");
            break;
        case SerialAccess::SerialOperationResult::success:
        default:
            break;
    }

    view->showSerialBubble(message);

    // Opens the Setup dialog in case of error.
    if (showSetupPanel) {
        auto& commandManager = mainController.getCommandManager();
        commandManager.invokeDirectly(CommandIds::setupSerial, true);
    }
}


// SubsongMetadataObserver method implementations.
// ================================================

void MainToolbarControllerImpl::onSubsongMetadataChanged(const Id& /*subsongId*/, const unsigned what)
{
    // Highlight, or replay frequency change?
    if (NumberUtil::isBitPresent(what, subsongMetadata)) {
        onNewSpeed(cachedSpeed);        // Forces the recalculation of the BPM and displays it.
    }
}

}   // namespace arkostracker
