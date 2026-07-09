#include "InstrumentAction.h"

#include "../../../controllers/MainController.h"
#include "../../../controllers/SongController.h"
#include "../../song/tool/ChangeInstrumentsOrdering.h"
#include "../../song/tool/browser/CellBrowser.h"
#include "../../song/tool/browser/SpecialCellBrowser.h"

namespace arkostracker 
{

InstrumentAction::InstrumentAction(SongController& pSongController) noexcept :
        songController(pSongController),
        song(songController.getSong()),
        modifiedCells(),
        modifiedSpecialCells()
{
}

void InstrumentAction::setSelectedInstrument(const int index, const bool forceSingleSelection) const noexcept
{
    auto& mainController = songController.getMainController();
    auto instrumentId = songController.getInstrumentId(index);
    // After a Delete, the index doesn't exist anymore if it was the last one.
    // It is more user-friendly to select the last one instead of nothing.
    if (instrumentId.isAbsent()) {
        instrumentId = songController.getLastInstrumentId();
    }

    mainController.setSelectedInstrumentId(instrumentId, forceSingleSelection);
}

void InstrumentAction::notifyGlobalChange() const noexcept
{
    // The PV will also observe this.
    songController.getInstrumentObservers().applyOnObservers([](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentsInvalidated();
    });
}

int InstrumentAction::getInstrumentCount() const noexcept
{
    return songController.getInstrumentCount();
}

bool InstrumentAction::performOnMapper(Remapper<Instrument>& remapper) noexcept
{
    // First, changes the Instruments themselves.
    std::unordered_set<int> sampleIndexes;
    songController.performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) noexcept {
        // Before, stores the position of the Samples.
        auto instrumentIndex = 0;
        for (const auto& instrument : instruments) {
            if (instrument->getType() == InstrumentType::sampleInstrument) {
                sampleIndexes.insert(instrumentIndex);
            }
            ++instrumentIndex;
        }
        // Changes the Instruments mapping.
        remapper.applyOnVector(instruments);
    });

    // Then, changes the mapping in the Tracks.
    const auto& mapping = remapper.getMapping();
    const auto cellsAndSpecialCells = ChangeInstrumentsOrdering::changeInstrumentsInSong(*song, mapping, sampleIndexes);
    modifiedCells = cellsAndSpecialCells.first;
    modifiedSpecialCells = cellsAndSpecialCells.second;

    notifyGlobalChange();

    return true;
}

bool InstrumentAction::undoOnMapper(Remapper<Instrument>& remapper) noexcept
{
    // Changes the ordering.
    songController.performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) noexcept {
        remapper.undoOnVector(instruments);
    });

    // Changes the Cells/Special Cells.
    const CellBrowser cellBrowser(*song);
    cellBrowser.writeCells(modifiedCells);
    SpecialCellBrowser::writeSpecialCells(*song, modifiedSpecialCells);

    modifiedCells.clear();
    modifiedSpecialCells.clear();

    notifyGlobalChange();

    return true;
}

}   // namespace arkostracker
