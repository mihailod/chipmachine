#pragma once

#include <set>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"
#include "../../model/Loop.h"

namespace arkostracker 
{

class SongController;

/**
 * Action to move Positions. This is typically used by drag'n'drop.
 * The original positions may be sparse, but they will be put in order, linearly, on the destination.
 */
class MovePositions final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positions the Positions to move.
     * @param destinationPosition the original position where to put them.
     * @param insertAfter true if the Potions must be moved after the destination, false if before.
     */
    MovePositions(SongController& songController, Id subsongId, std::set<int> positions, int destinationPosition, bool insertAfter) noexcept;

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

    const std::set<int> positionsToMove;
    std::vector<Position> movedPositions;       // Used for the Redo. In the same order as in the Set!
    const int originalDestinationPosition;
    Loop originalLoop;
    const bool insertAfter;

    int finalDestinationPosition;               // The corrected destination where the removed Positions are put. Also useful for Undo.

};

}   // namespace arkostracker
