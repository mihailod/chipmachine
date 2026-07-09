#include "MoveInstruments.h"

#include "../../../controllers/SongController.h"

namespace arkostracker 
{

MoveInstruments::MoveInstruments(SongController& pSongController, std::set<int> pIndexesToMove, const int pTargetIndex) noexcept :
        InstrumentAction(pSongController),
        MoveItems(std::move(pIndexesToMove), pTargetIndex)
{
}

int MoveInstruments::getItemCount() const noexcept
{
    return songController.getInstrumentCount();
}

void MoveInstruments::setSelectedItem(const int index, const bool forceSingleSelection) noexcept
{
    setSelectedInstrument(index, forceSingleSelection);
}

bool MoveInstruments::perform()
{
    return performAction();
}

bool MoveInstruments::undo()
{
    return undoAction();
}

bool MoveInstruments::performActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool MoveInstruments::undoActionOnMapper(Remapper<Instrument>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}

}   // namespace arkostracker
