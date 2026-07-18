#include "DeleteInstruments.h"

#include "../../../controllers/SongController.h"

namespace arkostracker 
{

DeleteInstruments::DeleteInstruments(SongController& pSongController, std::set<int> pIndexesToDelete) noexcept:
        InstrumentAction(pSongController),
        DeleteItems(std::move(pIndexesToDelete))
{
}

bool DeleteInstruments::perform()
{
    return performAction();
}

bool DeleteInstruments::undo()
{
    return undoAction();
}


// ListAction method implementations.
// =================================================

int DeleteInstruments::getItemCount() const noexcept
{
    return getInstrumentCount();
}

void DeleteInstruments::setSelectedItem(const int index, const bool forceSingleSelection) noexcept
{
    setSelectedInstrument(index, forceSingleSelection);
}

bool DeleteInstruments::performActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool DeleteInstruments::undoActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}

}   // namespace arkostracker
