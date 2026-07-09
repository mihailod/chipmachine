#include "PsgInstrumentEditorControllerImpl.h"

#include "../../../../controllers/MainController.h"
#include "../../../../controllers/PlayerController.h"
#include "PsgInstrumentEditorControllerSpecificImpl.h"

namespace arkostracker 
{

PsgInstrumentEditorControllerImpl::PsgInstrumentEditorControllerImpl(MainController& pMainController) noexcept :
        EditorWithBarsControllerImpl(pMainController, *std::make_unique<Factory>(), AreaType::soundType),
        localMainController(pMainController)
{
    localMainController.getPlayerController().getSongPlayerObservers().addObserver(this);
}

PsgInstrumentEditorControllerImpl::~PsgInstrumentEditorControllerImpl()
{
    localMainController.getPlayerController().getSongPlayerObservers().removeObserver(this);
}


// EditorWithBarsControllerImpl method implementations.
// =======================================================

bool PsgInstrumentEditorControllerImpl::canHideRows() const noexcept
{
    return true;
}

bool PsgInstrumentEditorControllerImpl::showShift() const noexcept
{
    return false;
}

bool PsgInstrumentEditorControllerImpl::showIsLooping() const noexcept
{
    return true;
}

bool PsgInstrumentEditorControllerImpl::showIsRetrig() const noexcept
{
    return true;
}

bool PsgInstrumentEditorControllerImpl::canHideRow(const AreaType areaType) const noexcept
{
    return (areaType != AreaType::envelope);        // Only envelope cannot be hidden.
}


// Factory method implementations.
// ==================================

std::unique_ptr<EditorWithBarsControllerSpecific> PsgInstrumentEditorControllerImpl::Factory::buildSpecificController(MainController& mainController,
                                                                                                                      EditorWithBarsController& editorController) noexcept
{
    return std::make_unique<PsgInstrumentEditorControllerSpecificImpl>(mainController, editorController);
}

// SongPlayerObserver method implementations.
// =======================================================

void PsgInstrumentEditorControllerImpl::onInstrumentPlayed(const InstrumentPlayedInfo& info) noexcept
{
    const auto itemIdOptional = getCurrentlyShownItemId();
    if (itemIdOptional.isAbsent()) {
        return;
    }

    const auto mutedChannels = localMainController.getChannelMuteStates();

    std::vector<int> indexesToHighlight;

    // Extracts the indexes to highlight from the structure.
    for (const auto& [channelIndex, instrumentIdAndIndex] : info.getInfo()) {
        // If muted, ignores this channel.
        if (mutedChannels.find(channelIndex) != mutedChannels.cend()) {
            continue;
        }

        const auto& [instrumentId, highlightIndex] = instrumentIdAndIndex;

        if (instrumentId.isPresent() && (instrumentId.getValueRef() == itemIdOptional.getValueRef())) {
            jassert(highlightIndex >= 0);

            indexesToHighlight.push_back(highlightIndex);
        }
    }

    highlightBarIndexes(indexesToHighlight);
}

}   // namespace arkostracker
