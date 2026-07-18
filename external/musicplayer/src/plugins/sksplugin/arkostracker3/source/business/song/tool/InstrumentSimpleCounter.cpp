#include "InstrumentSimpleCounter.h"

#include "../../../song/Song.h"
#include "browser/CellBrowser.h"

namespace arkostracker 
{

OccurrenceMap<int> InstrumentSimpleCounter::countInstrument(const Song& song, const Id& subsongId) noexcept
{
    OccurrenceMap<int> occurrenceMap;

    // Finds all the notes by browsing the tracks.
    CellBrowser::browseConstSubsongCells(song, subsongId, true, [&](const Cell& cell) noexcept {
        // No instrument? Then nothing to do.
        if (cell.isNoteAndInstrument()) {
            const auto instrumentIndex = cell.getInstrument().getValue();
            occurrenceMap.addItem(instrumentIndex);
        }

        return false;
    });

    return occurrenceMap;
}

}   // namespace arkostracker
