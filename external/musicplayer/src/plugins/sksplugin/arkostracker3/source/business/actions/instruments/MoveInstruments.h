#pragma once

#include "../../../song/instrument/Instrument.h"
#include "../lists/MoveItems.h"
#include "InstrumentAction.h"

namespace arkostracker 
{

class SongController;

/** Action to move Instruments. This also shifts the others and modifies the song. */
class MoveInstruments final : public InstrumentAction,
                              MoveItems<Instrument>
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param indexesToMove the indexes to move.
     * @param targetIndex the new index. The items will be put one after the other, in order. Does NOT take in account the fact that the expression has moved! So, matches the UI.
     */
    MoveInstruments(SongController& songController, std::set<int> indexesToMove, int targetIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    // MoveItems method implementations.
    // =================================================
    int getItemCount() const noexcept override;
    void setSelectedItem(int index, bool forceSingleSelection) noexcept override;
    bool performActionOnMapper(Remapper<Instrument>& remapper) noexcept override;
    bool undoActionOnMapper(Remapper<Instrument>& remapper) noexcept override;
};

}   // namespace arkostracker
