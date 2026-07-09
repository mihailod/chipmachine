#pragma once

#include "../../../../controllers/observers/GeneralDataObserver.h"
#include "../../../../controllers/observers/SongPlayerObserver.h"
#include "../../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../../keyboard/CommandIds.h"
#include "../view/MainToolbarView.h"
#include "MainToolbarController.h"

namespace arkostracker
{

class MainController;
class SongController;
class StorageListener;

/** Implementation of the Main Toolbar Controller. */
class MainToolbarControllerImpl final : public MainToolbarController,
                                        juce::Timer,
                                        public SongPlayerObserver,
                                        public GeneralDataObserver,             // To observe when a layout is restored.
                                        public SubsongMetadataObserver          // If highlight changes, must recalculates the BPMs.
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit MainToolbarControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~MainToolbarControllerImpl() override;

    // MainToolbarController method implementations.
    // ================================================
    void onTimerClicked() noexcept override;
    void onUserWantsToRestoreArrangement(int arrangementNumber) override;
    void onUserWantsToPlaySongFromStart() noexcept override;
    void onUserWantsToPlaySong() noexcept override;
    void onUserWantsToPlayPatternFromStart() noexcept override;
    void onUserWantsToPlayPattern() noexcept override;
    void onUserWantsToStopPlay() noexcept override;
    void onUserWantsToSetOctave(int desiredOctave) noexcept override;
    void onUserWantsToStartStopSerial() noexcept override;

    // ParentViewLifeCycleAware method implementations.
    // ===================================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;

    // Timer method implementations.
    // ================================
    void timerCallback() override;

    // SongPlayerObserver method implementations.
    // =============================================
    void onPlayerNewLocations(const Locations& locations) noexcept override;
    void onNewSpeed(int newSpeed) noexcept override;
    void onPlayerPlayEvent(Event event) noexcept override;

    // GeneralDataObserver method implementations.
    // =============================================
    void onGeneralDataChanged(unsigned int whatChanged) override;
    void onSerialCommunicationEvent(SerialController::CommunicationState state, SerialAccess::SerialOperationResult reason) override;

    // SubsongMetadataObserver method implementations.
    // ================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned what) override;

private:
    static const int timerRefreshRateMs;

    /** Gets the location being played, calculates the timer and displays it. */
    void getDataAndUpdateTimer() const noexcept;

    /**
     * @return a displayable duration (mm:ss).
     * @param frameCount how many frames.
     * @param replayFrequency the replay frequency.
     * @param showMinus true to show a "-" before the duration.
     */
    static juce::String getDisplayedDuration(int frameCount, float replayFrequency, bool showMinus) noexcept;

    /**
     * Performs a registered command.
     * @param commandId the ID of the command to perform.
     */
    void perform(CommandIds commandId) const noexcept;

    /** @return the bpm of the song according to the given speed, and the played location, or 0 if couldn't (the song doesn't exist!). */
    int calculateBpm(int speed) const noexcept;

    MainController& mainController;
    StorageListener& storageListener;                           // Object that knows how to store/restore arrangements.

    Location playedLocation;
    bool showRemainingDuration;                                 // True to show the remaining duration, false to show the total.

    int cachedSpeed;

    std::unique_ptr<MainToolbarView> view;
};

}   // namespace arkostracker
