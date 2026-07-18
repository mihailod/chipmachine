#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "../../../utils/Id.h"
#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

class SongController;

/** Action to set the loop start/end. Nothing happens if they are the same. Notifies the listeners. */
class SetLoopStartAndEnd final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param loopStartPosition the loop start position.Must be valid.
     * @param endPosition the end position. Must be valid.
     * @param subsongIndex the id of the Subsong. Must be valid.
     */
    SetLoopStartAndEnd(SongController& songController, Id subsongId, int loopStartPosition, int endPosition) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;
    const Id subsongId;
    const int loopStartPosition;
    const int endPosition;

    int oldLoopStartPosition;
    int oldEndPosition;
};

}   // namespace arkostracker

