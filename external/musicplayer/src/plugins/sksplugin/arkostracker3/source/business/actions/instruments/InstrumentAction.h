#pragma once

#include <memory>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/instrument/Instrument.h"
#include "../../../utils/Remapper.h"
#include "../../song/tool/browser/CellAndLocation.h"
#include "../../song/tool/browser/SpecialCellAndLocation.h"

namespace arkostracker 
{

class Song;
class SongController;

/** Base Action for Instruments. */
class InstrumentAction : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to get the Song, and to notify the changes.
     */
    explicit InstrumentAction(SongController& songController) noexcept;

protected:
    /**
     * Selects an Instrument. It will be corrected.
     * @param index the index (>0).
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after expressions are deleted, for example.
     */
    void setSelectedInstrument(int index, bool forceSingleSelection) const noexcept;

    /** Notifies the observers about global change. */
    void notifyGlobalChange() const noexcept;

    /** Performs the action using the given remapper, which must have been setup before. */
    bool performOnMapper(Remapper<Instrument>& remapper) noexcept;
    /** Undoes the action using the given remapper, which must have been setup before. */
    bool undoOnMapper(Remapper<Instrument>& remapper) noexcept;

    /** @return how many instruments there are. */
    int getInstrumentCount() const noexcept;

    SongController& songController;
    std::shared_ptr<Song> song;
    std::vector<CellAndLocation> modifiedCells;                     // Filled for undo.
    std::vector<SpecialCellAndLocation> modifiedSpecialCells;       // Filled for undo.
};


}   // namespace arkostracker

