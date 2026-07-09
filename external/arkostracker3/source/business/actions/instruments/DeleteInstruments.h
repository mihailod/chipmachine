#pragma once

#include "../../../song/instrument/Instrument.h"
#include "../lists/DeleteItems.h"
#include "InstrumentAction.h"

namespace arkostracker 
{

class SongController;

/** Action to delete Instruments. */
class DeleteInstruments final : public InstrumentAction,
                                DeleteItems<Instrument>
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param indexesToDelete the indexes of the Expressions to delete.
     */
    DeleteInstruments(SongController& songController, std::set<int> indexesToDelete) noexcept;

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
