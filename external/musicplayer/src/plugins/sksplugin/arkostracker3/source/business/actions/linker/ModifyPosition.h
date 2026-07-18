#pragma once

#include <memory>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to modify a Position (replace a Position with another one). */
class ModifyPosition final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positionIndex the index of the Position to modify. Must be valid.
     * @param newPosition the Position to replace the old Position with.
     */
    ModifyPosition(SongController& songController, Id subsongId, int positionIndex, Position newPosition) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionIndex;
    const Position newPosition;

    std::unique_ptr<Position> oldPosition;
};

}   // namespace arkostracker

