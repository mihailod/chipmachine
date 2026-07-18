#pragma once

#include <set>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"
#include "../../model/Loop.h"

namespace arkostracker 
{

class SongController;

/** Action to delete Positions. */
class DeletePositions final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positions the positions to delete.
     */
    DeletePositions(SongController& songController, Id subsongId, std::set<int> positions) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the Listeners of a change.
     * @param highlightedItems the items to select after the action.
     */
    void notifyListeners(const std::set<int>& highlightedItems) const noexcept;

    SongController& songController;

    const Id subsongId;

    const std::set<int> positionsToDelete;
    std::vector<Position> deletedPositions;     // Used for the Redo. In the same order as in the Set!
    Loop originalLoop;
};

}   // namespace arkostracker
