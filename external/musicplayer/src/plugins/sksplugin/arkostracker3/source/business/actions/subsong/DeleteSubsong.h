#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Subsong.h"
#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to delete a Subsong. */
class DeleteSubsong final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param subsongId the id of the Subsong to delete. Should be valid.
     */
    DeleteSubsong(SongController& songController, const Id& subsongId) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the listeners and shows the Subsong which id is given.
     * @param subsongIdToShow the id of the Subsong to show.
     */
    void notifyAndShowSubsong(const Id& subsongIdToShow) const noexcept;

    SongController& songController;
    const Id subsongIdToDelete;
    Id initialSubsongId;
    int deleteSubsongIndex;
    std::unique_ptr<Subsong> deletedSubsong;
};

}   // namespace arkostracker

