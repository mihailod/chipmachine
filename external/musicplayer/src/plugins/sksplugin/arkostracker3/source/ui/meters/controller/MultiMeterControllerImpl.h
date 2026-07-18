#pragma once

#include <memory>
#include <vector>

#include "MetersController.h"

#include "../../../controllers/observers/ChannelMuteObserver.h"
#include "../../../controllers/observers/SongMetadataObserver.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../../song/psg/Psg.h"
#include "../ChannelView.h"

namespace arkostracker 
{

class MainController;
class SongController;
class AudioController;

/**
 * Implementation of the MetersController, with several meters.
 *
 * The Controller is active: upon a timer, it will ask for the view buffers to be filled. This greatly simplifies the code and
 * the threading aspect.
 */
class MultiMeterControllerImpl final : public MetersController,
                                       public SongMetadataObserver,            // To be aware of new Subsong selection.
                                       public SubsongMetadataObserver,         // To be aware of new channel count.
                                       public ChannelMuteObserver,             // To be aware of new mute states.
                                       public juce::Timer                      // To refresh the UI.
{
public:
    /**
     * Constructor.
     * @param parentComponent where the views are added.
     * @param mainController the Controller to which registers observations and actions.
     */
    MultiMeterControllerImpl(juce::Component& parentComponent, MainController& mainController) noexcept;

    /** Destructor. */
    ~MultiMeterControllerImpl() override;

    // MetersController method implementations.
    // ==============================================
    void updateViewLocations(int startX, int startY, int width, int height) override;
    void onWantToChangeMuteState(int channelIndex, bool newMuteState) override;
    void onWantToAllMuteExcept(int channelIndex) override;
    void onUserWantsToSwitchView() noexcept override;

    // SongMetadataObserver method implementations.
    // ==============================================
    void onSongMetadataChanged(unsigned int what) override;

    // SubsongMetadataObserver method implementations.
    // ================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) override;

    // ChannelMuteObserver method implementations.
    // ==============================================
    void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) override;

    // juce::Timer method implementations.
    // ==============================================
    void timerCallback() override;

private:
    static const int refreshMs;                                         // How often is the display updated.

    /** Updates the views location from the already given startX/Y/width/height. */
    void updateViewLocations() const noexcept;

    /** Creates/deletes Channels and their type if needed, but only if needed. */
    void updateChannelsIfNeeded() noexcept;

    juce::Component& parentComponent;                                   // Where the views are added.
    MainController& mainController;                                     // The Controller to which registers observations and actions.
    SongController& songController;                                     // The Controller to which registers observations and actions.
    AudioController& audioController;                                   // The Controller to get the signal we want to draw.
    std::vector<std::unique_ptr<ChannelView>> channelViews;             // The channel to display.
    std::vector<Psg> shownPsgs;                                         // The PSGs that are shown.

    int startX;
    int startY;
    int availableWidth;
    int availableHeight;
};

}   // namespace arkostracker
