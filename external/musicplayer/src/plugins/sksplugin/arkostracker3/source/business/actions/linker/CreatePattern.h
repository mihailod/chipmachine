#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to create a new Position, with a new Pattern. */
class CreatePattern final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param sourcePositionIndex index after which to insert the new position.
     */
    CreatePattern(SongController& songController, Id subsongId, int sourcePositionIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the Listeners of a change.
     * @param highlightedPositionIndex the index of the position to highlight after the action.
     */
    void notifyListeners(int highlightedPositionIndex) const noexcept;

    SongController& songController;

    const Id subsongId;
    const int sourcePositionIndex;
};

}   // namespace arkostracker
