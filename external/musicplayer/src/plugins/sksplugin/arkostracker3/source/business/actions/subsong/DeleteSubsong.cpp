#include "DeleteSubsong.h"

#include "../../../controllers/SongController.h"
#include "../../../utils/NumberUtil.h"
#include "SubsongActionHelper.h"

namespace arkostracker 
{

DeleteSubsong::DeleteSubsong(SongController& pSongController, const Id& pSubsongId) noexcept :
        songController(pSongController),
        subsongIdToDelete(pSubsongId),
        initialSubsongId(pSubsongId),
        deleteSubsongIndex(0),
        deletedSubsong()
{
}

bool DeleteSubsong::perform()
{
    const auto subsongCount = songController.getSubsongCount();
    if (subsongCount <= 1) {
        jassertfalse;           // Never try to remove the last remaining Subsong!
        return false;
    }

    // Stores where we are before deleting.
    initialSubsongId = songController.getCurrentSubsongId();

    // Where to go?
    const auto song = songController.getSong();
    const auto deleteSubsongIndexOptional = song->getSubsongIndex(subsongIdToDelete);
    if (deleteSubsongIndexOptional.isAbsent()) {
        jassertfalse;           // Unable to determine the index of the Subsong to delete!
        return false;
    }
    deleteSubsongIndex = deleteSubsongIndexOptional.getValue();

    const auto subsongIdToGoToAfterDelete = song->getSubsongIdIfSubsongDeleted(subsongIdToDelete);
    if (subsongIdToGoToAfterDelete.isAbsent()) {
        jassertfalse;           // Unable to determine where to go after deletion!
        return false;
    }

    // Removes the Subsong, storing it locally.
    deletedSubsong = song->deleteSubsong(subsongIdToDelete);

    notifyAndShowSubsong(subsongIdToGoToAfterDelete.getValue());

    return true;
}

bool DeleteSubsong::undo()
{
    const auto success = songController.getSong()->insertSubsong(deleteSubsongIndex, std::move(deletedSubsong));
    if (success) {
        notifyAndShowSubsong(initialSubsongId);
    }

    return success;
}

void DeleteSubsong::notifyAndShowSubsong(const Id& subsongIdToShow) const noexcept
{
    SubsongActionHelper::notifyAndShowSubsong(songController, subsongIdToShow);
}

}   // namespace arkostracker
