#pragma once

#include <set>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to duplicate a Position. */
class DuplicatePositions final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positions the positions to duplicate.
     * @param destinationPosition where to insert the positions.
     * @param insertAfter true if the Positions must be put after the destination, false if before.
     * @param alsoDuplicateMarker true to also duplicate the marker. Most of the time, we don't want this.
     */
    DuplicatePositions(SongController& songController, Id subsongId, std::set<int> positions, int destinationPosition,
                       bool insertAfter, bool alsoDuplicateMarker) noexcept;

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
    const std::set<int> sourcePositions;
    const int destinationPosition;
    const bool insertAfter;
    const bool alsoDuplicateMarker;
};

}   // namespace arkostracker

