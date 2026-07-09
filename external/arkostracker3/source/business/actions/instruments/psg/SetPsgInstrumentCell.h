#pragma once

#include <unordered_map>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../../song/instrument/psg/PsgPart.h"

namespace arkostracker
{

class SongController;

/** Sets the Cell of a PSG Instrument. If out of bounds, spread the given cell (handy to the user). */
class SetPsgInstrumentCell final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the song controller.
     * @param id the Instrument ID. It must exist and be a PSG instrument.
     * @param cellIndex the cell index. May be out of bounds. If <0, nothing happens, but asserts as it shouldn't happen.
     * @param cell the new Cell. Will replace the previous one, or spread if out of bounds.
     */
    SetPsgInstrumentCell(SongController& songController, Id id, int cellIndex, const PsgInstrumentCell& cell) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;
    int cellIndex;
    int createdBarCount;                  // Useful to shrink the instrument in the redo.
    int refreshIndexStart;
    int originalEnd;
    int newEnd;
    std::unordered_map<PsgSection, Loop> originalAutoSpreadLoops;
    bool mustRefreshAllAfterToo;
    const PsgInstrumentCell newCell;
    PsgInstrumentCell oldCell;
};

}   // namespace arkostracker
