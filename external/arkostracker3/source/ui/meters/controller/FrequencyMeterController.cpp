#include "FrequencyMeterController.h"

#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"

namespace arkostracker
{

const int FrequencyMeterController::refreshMs = 30;

FrequencyMeterController::FrequencyMeterController(juce::Component& pParentComponent, MainController& pMainController) noexcept :
        mainController(pMainController),
        frequencyBarsView(*this),
        mutedChannelIndexes(mainController.getChannelMuteStates())
{
    mainController.getPlayerController().getSongPlayerObservers().addObserver(this);
    mainController.observers().getChannelMuteStateObservers().addObserver(this);

    startTimer(refreshMs);

    pParentComponent.addAndMakeVisible(frequencyBarsView);
}

FrequencyMeterController::~FrequencyMeterController()
{
    stopTimer();

    mainController.observers().getChannelMuteStateObservers().removeObserver(this);
    mainController.getPlayerController().getSongPlayerObservers().removeObserver(this);
}


// MetersController method implementations.
// ==============================================

void FrequencyMeterController::updateViewLocations(const int startX, const int startY, const int width, const int height)
{
    frequencyBarsView.setBounds(startX, startY, width, height);
}

void FrequencyMeterController::onWantToChangeMuteState(int /*channelIndex*/, bool /*newMuteState*/)
{
    // Nothing to do.
}

void FrequencyMeterController::onWantToAllMuteExcept(int /*channelIndex*/)
{
    // Nothing to do.
}


// SongPlayerObserver method implementations.
// ==============================================

void FrequencyMeterController::onNewPsgRegisters(const std::unordered_map<int, std::pair<PsgRegisters, SampleData>>& psgIndexToPsgRegistersAndSampleData) noexcept
{
    frequencyBarsView.onBeforeNewValues();

    for (const auto& [psgIndex, psgRegistersAndSampleData] : psgIndexToPsgRegistersAndSampleData) {
        const auto& psgRegisters = psgRegistersAndSampleData.first;

        for (auto channelIndexInPsg = 0; channelIndexInPsg < PsgValues::channelCountPerPsg; ++channelIndexInPsg) {
            // If mute, don't do anything.
            const auto channelIndexInSong = PsgValues::getChannelIndex(channelIndexInPsg, psgIndex);
            if (mutedChannelIndexes.find(channelIndexInSong) != mutedChannelIndexes.cend()) {
                continue;
            }

            // Any software period?
            const auto softwarePeriod = psgRegisters.getSoftwarePeriod(channelIndexInPsg);
            const auto volume = psgRegisters.getVolume(channelIndexInPsg);
            if (psgRegisters.getMixerSoundState(channelIndexInPsg)) {
                frequencyBarsView.addSoftwarePeriod(channelIndexInPsg, softwarePeriod, volume);
            }

            // Any hardware period?
            if (psgRegisters.isHardwareVolume(channelIndexInPsg)) {
                frequencyBarsView.addHardwarePeriod(psgRegisters.getHardwarePeriod());
            }

            // Any noise?
            if (psgRegisters.getMixerNoiseState(channelIndexInPsg)) {
                if (const auto noise = psgRegisters.getNoise(); noise > 0) {
                    frequencyBarsView.addNoise(channelIndexInPsg, noise, volume);
                }
            }
        }
    }
}


// ChannelMuteObserver method implementations.
// ==============================================

void FrequencyMeterController::onChannelsMuteStateChanged(const std::unordered_set<int>& pMutedChannelIndexes)
{
    mutedChannelIndexes = pMutedChannelIndexes;
}


// juce::Timer method implementations.
// ==============================================

void FrequencyMeterController::timerCallback()
{
    frequencyBarsView.update();
}


// FrequencyBarsView::Controller method implementations.
// ==============================================

int FrequencyMeterController::getChannelCount() const noexcept
{
    return mainController.getSongController().getChannelCount();
}

std::unordered_set<int> FrequencyMeterController::getMutedChannelIndexes() const noexcept
{
    return mainController.getChannelMuteStates();
}

void FrequencyMeterController::onUserWantsToMuteChannel(const int channelIndex) noexcept
{
    mainController.onWantToToggleMuteState(channelIndex);
}

void FrequencyMeterController::onUserWantsToSoloChannel(const int channelIndex) noexcept
{
    mainController.onWantToAllMuteExcept(channelIndex);
}

void FrequencyMeterController::onUserWantsToUnmuteAll() noexcept
{
    mainController.unmuteAll();
}

void FrequencyMeterController::onUserWantsToSwitchView() noexcept
{
    // Nothing to do.
}

}   // namespace arkostracker
