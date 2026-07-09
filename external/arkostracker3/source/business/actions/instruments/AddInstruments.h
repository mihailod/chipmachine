#pragma once

#include "../../../song/instrument/Instrument.h"
#include "../lists/AddItems.h"
#include "InstrumentAction.h"

namespace arkostracker 
{

class SongController;

/** Action to add Instruments. This also shifts the others and modifies the song. */
class AddInstruments final : public InstrumentAction,
                             AddItems<Instrument>
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param indexWhereToInsert the index where to insert.
     * @param originalSelectedIndexForUndo the selected index, only for Undo (to put the selection back).
     * @param instruments the instruments to insert.
     */
    AddInstruments(SongController& songController, int indexWhereToInsert, int originalSelectedIndexForUndo,
                   std::vector<std::unique_ptr<Instrument>> instruments) noexcept;

    bool perform() override;
    bool undo() override;

private:
    // DeleteItems method implementations.
    // =================================================
    int getItemCount() const noexcept override;
    void setSelectedItem(int index, bool forceSingleSelection) noexcept override;
    bool performActionOnMapper(Remapper<Instrument>& remapper) noexcept override;
    bool undoActionOnMapper(Remapper<Instrument>& remapper) noexcept override;
};

}   // namespace arkostracker

