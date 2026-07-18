#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"
#include "../../model/Loop.h"

namespace arkostracker
{

class SongController;

/**
 * Modifies the given Expression. This is useful to modify one to several Cells, including generation of Cells.
 * WARNING! Since the lambda is called also for the Redo, its data must be copied, not referenced!
 */
class ModifyExpressionCells final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param id the Instrument ID. It must exist and be a PSG instrument.
     * @param startCellIndex the cell index on which to start to work.
     * @param cellCountToWorkOn how many cells to modify. May be superior to what the instrument has. If 0, nothing happens.
     * @param correctLoop true to correct the loop (the end). False not to change it.
     * @param actionOnCell action to perform on each value. If out of bounds, an default value is given.
     */
    ModifyExpressionCells(SongController& songController, bool isArpeggio, Id id, int startCellIndex, int cellCountToWorkOn, bool correctLoop,
                         std::function<int(int iterationIndex, int originalValue)> actionOnCell) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;

    SongController& songController;
    const bool isArpeggio;
    const Id id;
    const int startCellIndex;
    const int cellCountToWorkOn;
    const bool correctLoop;
    std::function<int(int iterationIndex, int originalValue)> actionOnCell;

    Loop originalLoop;
    int addedCellCount;
    std::vector<int> modifiedCells;       // Not the ones that were added.
};

}   // namespace arkostracker
