#include "InstrumentsOptimizer.h"

#include "../CellOperations.h"
#include "../ChangeInstrumentsOrdering.h"
#include "../browser/CellBrowser.h"
#include "../browser/SpecialCellBrowser.h"
#include "InstrumentOptimizer.h"

namespace arkostracker
{

InstrumentsOptimizer::InstrumentsOptimizer(const Song& pOriginalSong, bool pKeepSamples, bool pFlatten, bool pTransferInstrumentRetrig) noexcept :
        originalSong(pOriginalSong),
        newSong(std::make_unique<Song>(pOriginalSong)),
        keepSamples(pKeepSamples),
        flatten(pFlatten),
        transferInstrumentRetrig(pTransferInstrumentRetrig)
{
    // Deletes all the Instruments of the new Song!
    newSong->performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) noexcept {
        instruments.clear();
    });
}

std::unique_ptr<Song> InstrumentsOptimizer::optimize() noexcept
{
    // Makes sure the illegal Instruments are converted to RST.
    checkInstrumentsValidity();

    constexpr auto stopAtLoopEnd = false;

    std::unordered_map<int, OptionalInt> oldInstrumentIndexToNew;       // If the old index is Absent, it means the Instrument is deleted.
    oldInstrumentIndexToNew.insert({ 0, 0 });                   // Instrument 0 is always present!

    std::set<int> oldInstrumentIndexUsed;                               // They may not exist!
    oldInstrumentIndexUsed.insert(0);                                 // Instrument 0 is always present.

    // Finds all the Instruments that are used.
    CellBrowser::browseConstSongCells(*newSong, stopAtLoopEnd, [&](const Cell& cell) {
        if (cell.isNoteAndInstrument()) {
            const auto instrument = cell.getInstrument().getValue();
            oldInstrumentIndexUsed.insert(instrument);
        }

        return false;
    });

    // Browses the Special Tracks to get more used Samples.
    std::unordered_set<int> sampleIndexes;
    if (keepSamples) {
        // Gathers the indexes of the sample Instrument.
        auto instrumentIndex = 0;
        originalSong.performOnConstInstruments([&] (const std::vector<std::unique_ptr<Instrument>>& instruments) {
            for (const auto& instrument : instruments) {
                if (instrument->getType() == InstrumentType::sampleInstrument) {
                    sampleIndexes.insert(instrumentIndex);
                }
                ++instrumentIndex;
            }
        });

        SpecialCellBrowser::browseConstSongSpecialCells(*newSong, false, stopAtLoopEnd, [&](const SpecialCell& specialCell) {
            if (!specialCell.isEmpty()) {
                // Checks if instrument is sample. We can NOT use a browser here, else it would provoke a deadlock.
                const auto sampleIndex = specialCell.getValue();
                if (sampleIndexes.find(sampleIndex) != sampleIndexes.cend()) {
                    oldInstrumentIndexUsed.insert(sampleIndex);
                }
            }
        });
    }

    std::vector<std::unique_ptr<Instrument>> newInstruments;

    // Browses each Instrument. The samples may be discarded.
    // Also, makes sure each browsed Instrument is not a duplicate of a previously inserted one.
    for (auto oldInstrumentIndex : oldInstrumentIndexUsed) {
        originalSong.performOnConstInstrumentFromIndex(oldInstrumentIndex, [&](const Instrument& originalInstrument) {
            if (!keepSamples && originalInstrument.getType() == InstrumentType::sampleInstrument) {
                // Must not be added to the final map.
                oldInstrumentIndexToNew.insert({ oldInstrumentIndex, { } });
            } else {
                auto newInstrumentIndex = static_cast<int>(newInstruments.size());
                auto newInstrument = std::make_unique<Instrument>(originalInstrument);
                // Before inserting the Instrument, optimizes it.
                InstrumentOptimizer optimizer(*newInstrument, transferInstrumentRetrig, flatten);
                optimizer.optimize();

                // Then makes sure there is no duplicate before inserting it.
                auto foundDuplicate = false;
                auto alreadyAddedInstrumentIndex = 0;
                for (const auto& alreadyAddedInstrument : newInstruments) {
                    if (newInstrument->isMusicallyEqualTo(*alreadyAddedInstrument)) {
                        foundDuplicate = true;
                        break;
                    }
                    ++alreadyAddedInstrumentIndex;
                }

                // Inserts it and fills the oldToNew map.
                if (!foundDuplicate) {
                    newInstruments.push_back(std::move(newInstrument));
                } else {
                    // If duplicate, the "new instrument" index is the one that matches.
                    newInstrumentIndex = alreadyAddedInstrumentIndex;
                }
                // In all cases, links the old Instrument index either to the new one, or the one that matches.
                oldInstrumentIndexToNew.insert({ oldInstrumentIndex, newInstrumentIndex });
            }
        });
    }

    // Puts the new Instruments in the new Song.
    newSong->performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (auto& instrument : newInstruments) {
            instruments.push_back(std::move(instrument));
        }
    });

    // Remaps.
    ChangeInstrumentsOrdering::changeInstrumentsInSong(*newSong, oldInstrumentIndexToNew, sampleIndexes);

    return std::move(newSong);
}

void InstrumentsOptimizer::checkInstrumentsValidity() const noexcept
{
    // The count is done on the *original* song, but the modification is done on the Tracks of the new Song.
    const auto instrumentCount = originalSong.getInstrumentCount();

    const CellBrowser cellBrowser(*newSong);
    cellBrowser.browseCells(false, IllegalInstrumentToRstInCell(instrumentCount));
}

}   // namespace arkostracker
