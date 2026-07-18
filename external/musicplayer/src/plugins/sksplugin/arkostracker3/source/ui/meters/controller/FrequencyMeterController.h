#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MetersController.h"
#include "../FrequencyBarsView.h"
#include "../../../controllers/observers/ChannelMuteObserver.h"
#include "../../../controllers/observers/SongPlayerObserver.h"

namespace arkostracker
{

class MainController;

class FrequencyMeterController final : public MetersController,
                                       public SongPlayerObserver,
                                       public ChannelMuteObserver,
                                       public juce::Timer,                     // To refresh the UI.
                                       public FrequencyBarsView::Controller
{
public:
    /**
     * Constructor.
     * @param parentComponent the parent Component.
     * @param mainController the main Controller.
     */
    FrequencyMeterController(juce::Component& parentComponent, MainController& mainController) noexcept;

    /** Destructor. */
    ~FrequencyMeterController() override;

    // MetersController method implementations.
    // ==============================================
    void updateViewLocations(int startX, int startY, int width, int height) override;
    void onWantToChangeMuteState(int channelIndex, bool newMuteState) override;
    void onWantToAllMuteExcept(int channelIndex) override;

    // SongPlayerObserver method implementations.
    // ==============================================
    void onNewPsgRegisters(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept override;

    // ChannelMuteObserver method implementations.
    // ==============================================
    void onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes) override;

    // juce::Timer method implementations.
    // ==============================================
    void timerCallback() override;

    // FrequencyBarsView::Controller method implementations.
    // ==============================================
    int getChannelCount() const noexcept override;
    std::unordered_set<int> getMutedChannelIndexes() const noexcept override;
    void onUserWantsToMuteChannel(int channelIndex) noexcept override;
    void onUserWantsToSoloChannel(int channelIndex) noexcept override;
    void onUserWantsToUnmuteAll() noexcept override;
    void onUserWantsToSwitchView() noexcept override;

private:
    static const int refreshMs;                                         // How often is the display updated.

    MainController& mainController;                                     // The Controller to which registers observations and actions.
    FrequencyBarsView frequencyBarsView;

    std::unordered_set<int> mutedChannelIndexes;
};

}   // namespace arkostracker
