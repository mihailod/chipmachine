#include "CreateSubsong.h"

#include "../../../controllers/SongController.h"
#include "SubsongActionHelper.h"

namespace arkostracker 
{

CreateSubsong::CreateSubsong(SongController& pSongController, Properties pMetadata, std::vector<Psg> pInputPsgs) noexcept :
        songController(pSongController),
        metadata(std::move(pMetadata)),
        psgs(std::move(pInputPsgs)),
        newSubsongId(),
        previousSubsongId()
{
    jassert(!psgs.empty());         // Must NEVER happen!
}

bool CreateSubsong::perform()
{
    // What Subsong is left? Useful to go back if Undo.
    previousSubsongId = songController.getCurrentSubsongId();

    // Creates a new Subsong.
    auto subsong = std::make_unique<Subsong>(metadata.getName(), metadata.getInitialSpeed(), metadata.getReplayFrequencyHz(), metadata.getDigiChannel(),
                                             metadata.getHighlightSpacing(), metadata.getSecondaryHighlight(), 0, 0,
                                             psgs, metadata.getSidPlayerCapability(), true);
    newSubsongId = subsong->getId();
    songController.getSong()->addSubsong(std::move(subsong));
    notifyAndShowSubsong(newSubsongId);

    return true;
}

bool CreateSubsong::undo()
{
    songController.getSong()->deleteSubsong(newSubsongId);
    notifyAndShowSubsong(previousSubsongId);

    return true;
}

void CreateSubsong::notifyAndShowSubsong(const Id& subsongIdToShow) const noexcept
{
    SubsongActionHelper::notifyAndShowSubsong(songController, subsongIdToShow);
}

}   // namespace arkostracker
