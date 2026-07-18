#include "AddInstruments.h"

#include "../../../controllers/MainController.h"

namespace arkostracker 
{

AddInstruments::AddInstruments(SongController& pSongController, const int pIndexWhereToInsert, const int pOriginalSelectedIndexForUndo,
                               std::vector<std::unique_ptr<Instrument>> pInstruments) noexcept :
        InstrumentAction(pSongController),
        AddItems(pIndexWhereToInsert, std::move(pInstruments), pOriginalSelectedIndexForUndo)
{
    jassert(pIndexWhereToInsert >= 0);
}

bool AddInstruments::perform()
{
    return performAction();
}

bool AddInstruments::undo()
{
    return undoAction();
}

int AddInstruments::getItemCount() const noexcept
{
    return songController.getInstrumentCount();
}

void AddInstruments::setSelectedItem(const int index, const bool forceSingleSelection) noexcept
{
    setSelectedInstrument(index, forceSingleSelection);
}

bool AddInstruments::performActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool AddInstruments::undoActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}


}   // namespace arkostracker

