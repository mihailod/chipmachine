#include "FirstNotePerInstrumentFinder.h"

#include "../../../../song/Song.h"
#include "../browser/CellBrowser.h"

namespace arkostracker
{

FirstNotePerInstrumentFinder::ResultItem::ResultItem(OptionalInt pFoundNote, bool pExported) noexcept:
        foundNote(pFoundNote),
        exported(pExported)
{
}

OptionalInt FirstNotePerInstrumentFinder::ResultItem::getFoundNote() const noexcept
{
    return foundNote;
}

bool FirstNotePerInstrumentFinder::ResultItem::isExported() const noexcept
{
    return exported;
}

bool FirstNotePerInstrumentFinder::ResultItem::canUseUsed() const noexcept
{
    return foundNote.isPresent() && exported;
}

// ===================================================

std::map<int, FirstNotePerInstrumentFinder::ResultItem> FirstNotePerInstrumentFinder::find(const Song& song) noexcept
{
    std::map<int, FirstNotePerInstrumentFinder::ResultItem> instrumentIndexToSfxResult;

    // Parses the Instruments. Only 0 and samples are discarded.
    // Makes a search for each Instrument. Not particularly efficient, but simplifies the algorithm.
    song.performOnConstInstruments([&](const std::vector<std::unique_ptr<Instrument>>& instruments) {
        auto instrumentIndex = 0;
        for (const auto& instrument: instruments) {
            if ((instrumentIndex > 0) && (instrument->getType() == InstrumentType::psgInstrument)) {
                findInstrument(song, instrumentIndex, instrument->getConstPsgPart().isSoundEffectExported(), instrumentIndexToSfxResult);
            }

            ++instrumentIndex;
        }
    });

    return instrumentIndexToSfxResult;
}

void FirstNotePerInstrumentFinder::findInstrument(const Song& song, int instrumentIndex, bool isSfxInstrumentExported,
                                                  std::map<int, FirstNotePerInstrumentFinder::ResultItem>& instrumentIndexToSfxResultToFill) noexcept
{
    auto found = false;

    // Browses the whole Song, stopping when all the Instruments are found.
    CellBrowser::browseConstSongCells(song, true, [&](const Cell& cell) {
        // Does the Instrument is the one we're looking for?
        if (cell.isNoteAndInstrument() && (cell.getInstrument() == instrumentIndex)) {
            // Gets the exported flag from the Instrument, and uses the found note.
            const auto note = cell.getNote().getValue().getNote();
            instrumentIndexToSfxResultToFill.insert({ instrumentIndex, { note, isSfxInstrumentExported }});
            found = true;
        }

        // Stops if we found the Instrument.
        return found;
    });

    // If the Instrument is not found, fills the slot with "not found", yet keeps the original "is exported" flag.
    if (!found) {
        instrumentIndexToSfxResultToFill.insert({ instrumentIndex, { OptionalInt(), isSfxInstrumentExported }});
    }
}

}   // namespace arkostracker
