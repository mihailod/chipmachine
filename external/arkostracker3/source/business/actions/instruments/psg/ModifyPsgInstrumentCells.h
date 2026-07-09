#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../model/Loop.h"

namespace arkostracker
{

class SongController;

/**
 * Modifies the given Cell. This is useful to modify one to several Cells, including generation of Cells.
 * WARNING! Since the lambda is called also for the Redo, its data must be copied, not referenced!
 */
class ModifyPsgInstrumentCells final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the Instrument ID. It must exist and be a PSG instrument.
     * @param startCellIndex the cell index on which to start to work.
     * @param cellCountToWorkOn how many cells to modify. May be superior to what the instrument has. If 0, nothing happens.
     * @param correctLoop true to correct the loop (the end). False not to change it.
     * @param actionOnCell action to perform on each Cell. If out of bounds, an empty cell is given. The caller must
     * return another Cell.
     */
    ModifyPsgInstrumentCells(SongController& songController, Id id, int startCellIndex, int cellCountToWorkOn, bool correctLoop,
                             std::function<PsgInstrumentCell(int iterationIndex, const PsgInstrumentCell& cell)> actionOnCell) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;
    const int startCellIndex;
    const int cellCountToWorkOn;
    const bool correctLoop;
    std::function<PsgInstrumentCell(int iterationIndex, const PsgInstrumentCell& cell)> actionOnCell;

    Loop originalLoop;
    int addedCellCount;
    std::vector<PsgInstrumentCell> modifiedCells;       // Not the ones that were added.
};

}   // namespace arkostracker
