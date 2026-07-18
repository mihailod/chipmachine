#include "MultiMeterControllerImpl.h"

#include <unordered_set>

#include "../../../controllers/AudioController.h"
#include "../../../controllers/MainController.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "../MetersArranger.h"

namespace arkostracker 
{

const int MultiMeterControllerImpl::refreshMs = 40;

MultiMeterControllerImpl::MultiMeterControllerImpl(juce::Component& pParentComponent, MainController& pMainController) noexcept:
        parentComponent(pParentComponent),
        mainController(pMainController),
        songController(pMainController.getSongController()),
        audioController(pMainController.getAudioController()),
        channelViews(),
        shownPsgs(),
        startX(),
        startY(),
        availableWidth(),
        availableHeight()
{
    // Observes the song change to be aware of changes in mute state/channel count change.
    songController.getSongMetadataObservers().addObserver(this);
    songController.getSubsongMetadataObservers().addObserver(this);
    mainController.observers().getChannelMuteStateObservers().addObserver(this);

    // Shows the channels.
    updateChannelsIfNeeded();

    startTimer(refreshMs);
}

MultiMeterControllerImpl::~MultiMeterControllerImpl()
{
    stopTimer();

    songController.getSongMetadataObservers().removeObserver(this);
    songController.getSubsongMetadataObservers().removeObserver(this);
    mainController.observers().getChannelMuteStateObservers().removeObserver(this);
}


// MetersController method implementations.
// ==============================================

void MultiMeterControllerImpl::updateViewLocations(const int pStartX, const int pStartY, const int width, const int height)
{
    startX = pStartX;
    startY = pStartY;
    availableWidth = width;
    availableHeight = height;

    updateViewLocations();
}

void MultiMeterControllerImpl::onWantToChangeMuteState(const int channelIndex, const bool newMuteState)
{
    mainController.onWantToChangeMuteState(channelIndex, newMuteState);
}

void MultiMeterControllerImpl::onWantToAllMuteExcept(const int channelIndex)
{
    mainController.onWantToAllMuteExcept(channelIndex);
}

void MultiMeterControllerImpl::onUserWantsToSwitchView() noexcept
{
    // Nothing to do.
}


// SongMetadataObserver method implementations.
// ==============================================

void MultiMeterControllerImpl::onSongMetadataChanged(const unsigned int what)
{
    // Only interested in the Selected Subsong.
    if (!NumberUtil::isBitPresent(what, SongMetadataObserver::What::shownLocation)) {
        return;
    }
    updateChannelsIfNeeded();
}

// SubsongMetadataObserver method implementations.
// ==============================================

void MultiMeterControllerImpl::onSubsongMetadataChanged(const Id& /*subsongId*/, const unsigned int what)
{
    // Only interested in the PSG count and type.
    if (!NumberUtil::isBitPresent(what, SubsongMetadataObserver::What::psgsData)) {
        return;
    }
    updateChannelsIfNeeded();
}


// ChannelMuteObserver method implementations.
// ==============================================

void MultiMeterControllerImpl::onChannelsMuteStateChanged(const std::unordered_set<int>& mutedChannelIndexes)
{
    for (const auto& channelView : channelViews) {
        // Is this channel muted?
        const auto isMuted = (mutedChannelIndexes.find(channelView->getChannelIndex()) != mutedChannelIndexes.cend());
        channelView->changeMuteState(isMuted);
    }
}


// ==============================================

void MultiMeterControllerImpl::updateViewLocations() const noexcept
{
    // Calculates the new bounds.
    auto locations = MetersArranger::findBounds(static_cast<int>(channelViews.size()), startX, startY, availableWidth, availableHeight);
    if (locations.empty()) {
        // Happens on init, when there is no dimension yet, or when switching the meter view.
        return;
    }

    jassert(locations.size() == channelViews.size());
    // Applies the new locations.
    size_t index = 0;
    for (const auto& location : locations) {
        channelViews.at(index)->setBounds(location);
        ++index;
    }
}

void MultiMeterControllerImpl::updateChannelsIfNeeded() noexcept
{
    // What are the PSGs to show?
    const auto currentSubsong = mainController.getSongController().getCurrentSubsongId();
    const auto newPsgs = songController.getPsgs(currentSubsong);
    // Any difference with the ones already shown? If not, exits.
    if (newPsgs == shownPsgs) {
        return;
    }
    shownPsgs = newPsgs;

    // What channels are muted?
    auto mutedChannels = mainController.getChannelMuteStates();

    // Adds more channels or removes the lasts.
    const auto newChannelCount = PsgValues::getChannelCount(static_cast<int>(newPsgs.size()));
    const auto previousCount = static_cast<int>(channelViews.size());

    if (newChannelCount < previousCount) {
        // Removes exceeding meters.
        channelViews.resize(static_cast<size_t>(newChannelCount));
    } else if (newChannelCount > previousCount) {
        // Adds new meters.
        const auto signalBufferSize = audioController.getSignalBufferSize();

        const auto countToAdd = newChannelCount - previousCount;
        for (auto i = 0; i < countToAdd; ++i) {
            const auto channelIndex = static_cast<int>(channelViews.size());
            const bool muted = (mutedChannels.find(channelIndex) != mutedChannels.cend());

            // We don't care about their type, it will be changed just after.
            auto channelView = std::make_unique<ChannelView>(*this, channelIndex, PsgType::ay, muted, signalBufferSize);
            parentComponent.addAndMakeVisible(*channelView);

            channelViews.push_back(std::move(channelView));
        }
    }

    // Another pass on their type.
    auto channelIndex = 0;
    jassert(static_cast<int>(channelViews.size()) == newChannelCount);
    for (const auto& channelView : channelViews) {
        const auto psgIndex = static_cast<size_t>(PsgValues::getPsgIndex(channelIndex));
        const auto psgType = newPsgs.at(psgIndex).getType();
        channelView->setPsgStyle(psgType);

        ++channelIndex;
    }

    updateViewLocations();
}


// juce::Timer method implementations.
// ==============================================

void MultiMeterControllerImpl::timerCallback()
{
    // Builds the vector of the buffers to fill.
    std::vector<juce::HeapBlock<unsigned short>*> buffers;
    buffers.reserve(channelViews.size());

    for (const auto& channelView : channelViews) {
        auto& channelBuffer = channelView->getDisplayBuffer();
        buffers.push_back(&channelBuffer);
    }

    // Asks to fill these buffers with the signal to draw.
    audioController.fillSignal(buffers);

    // Draws the buffers!
    for (const auto& channelView : channelViews) {
        channelView->refreshChannelView();
    }
}

}   // namespace arkostracker
