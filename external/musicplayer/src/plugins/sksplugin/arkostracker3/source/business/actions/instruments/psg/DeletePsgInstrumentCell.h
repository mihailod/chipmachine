#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../../song/instrument/psg/PsgSection.h"
#include "../../../model/Loop.h"

namespace arkostracker
{

class SongController;

/** Deletes a PSG Instrument Cell. Nothing happens if it is the same one an Instrument possesses. */
class DeletePsgInstrumentCell final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the song controller.
     * @param id the Instrument ID. It must exist and be a PSG instrument.
     * @param cellIndex the cell index. May be out of bounds. If <0, nothing happens, but asserts as it shouldn't happen.
     */
    DeletePsgInstrumentCell(SongController& songController, Id id, int cellIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;
    const int cellIndex;
    Loop originalLoop;
    std::unordered_map<PsgSection, Loop> originalAutoSpreadLoops;
    PsgInstrumentCell originalCell;
};

}   // namespace arkostracker
