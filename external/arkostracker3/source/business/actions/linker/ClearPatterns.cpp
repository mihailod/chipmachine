#include "ClearPatterns.h"

#include "../../../controllers/SongController.h"
#include "../subsong/SubsongActionHelper.h"

namespace arkostracker
{

ClearPatterns::ClearPatterns(SongController& pSongController, Id pSubsongId) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        newSubsongId(),
        storedSubsong()
{
}

bool ClearPatterns::perform()
{
    const auto song = songController.getSong();
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Copies all the data for the Undo.
        storedSubsong = std::make_unique<Subsong>(subsong);
    });

    // Creates a new Subsong with the same basic metadata.
    const auto metadata = storedSubsong->getMetadata();
    auto newSubsong = std::make_unique<Subsong>(metadata.getName(), metadata.getInitialSpeed(), metadata.getReplayFrequencyHz(), metadata.getDigiChannel(),
        metadata.getHighlightSpacing(), metadata.getSecondaryHighlight(), 0, 0, storedSubsong->getPsgs(), metadata.getSidPlayerCapability(), true);
    newSubsongId = newSubsong->getId();

    song->replaceSubsong(subsongId, std::move(newSubsong));

    notify(newSubsongId);

    return true;
}

bool ClearPatterns::undo()
{
    if (storedSubsong == nullptr) {
        jassertfalse;       // Abnormal!
        return false;
    }

    const auto song = songController.getSong();
    const auto oldSubsongId = storedSubsong->getId();

    song->replaceSubsong(newSubsongId, std::move(storedSubsong));
    storedSubsong.reset();

    notify(oldSubsongId);

    return true;
}

void ClearPatterns::notify(const Id& subsongIdToShow) const noexcept
{
    SubsongActionHelper::notifyAndShowSubsong(songController, subsongIdToShow);
}

}   // namespace arkostracker
