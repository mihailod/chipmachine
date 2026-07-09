#include "ChangeInstrumentsOrdering.h"

#include "browser/CellBrowser.h"
#include "browser/SpecialCellBrowser.h"

namespace arkostracker 
{

std::pair<std::vector<CellAndLocation>, std::vector<SpecialCellAndLocation>> ChangeInstrumentsOrdering::changeInstrumentsInSong(
        Song& song, const std::unordered_map<int, OptionalInt>& mapping,
        const std::unordered_set<int>& oldSampleIndexesForSpecialEvent) noexcept
{
    CellBrowser const cellBrowser(song);

    // Browses all the Cells, changes the ones that needs to. Returns the original ones.
    const auto cellsAndLocations = cellBrowser.browseCells(true, [&](const Cell& originalCell) -> std::unique_ptr<Cell> {
        auto modified = false;

        // Does the cell has an instrument?
        if (originalCell.isNoteAndInstrument()) {
            auto cell = originalCell;
            const auto instrumentIndex = cell.getInstrument().getValue();

            // The instrument must be remapped or deleted. If not found, don't change it.
            if (const auto iterator = mapping.find(instrumentIndex); iterator != mapping.cend()) {
                if (const auto newInstrumentIndex = iterator->second; newInstrumentIndex.isAbsent()) {
                    // The instrument is absent, meaning it must be deleted, but also the note, else it will be transformed into a legato.
                    cell.clearNote();
                    cell.clearInstrument();
                    modified = true;
                } else if (newInstrumentIndex.getValue() != instrumentIndex) {
                    // Changes the instrument, it is different.
                    cell.setInstrument(newInstrumentIndex);

                    modified = true;
                }
            }

            // Returns a modified Cell.
            if (modified) {
                return std::make_unique<Cell>(cell);
            }
        }

        // Not modified.
        jassert(!modified);
        return nullptr;
    });

    // The same with the Special Cells.
    std::vector<SpecialCellAndLocation> specialCellsAndLocations;
    if (!oldSampleIndexesForSpecialEvent.empty()) {
        specialCellsAndLocations = SpecialCellBrowser::browseSpecialCells(song, false, false,
              [&](const SpecialCell& originalCell) -> std::unique_ptr<SpecialCell> {
              // Any value?
              if (originalCell.isEmpty()) {
                  return nullptr;
              }
              // The value is matching a sample?
              const auto sampleIndex = originalCell.getValue();
              if (oldSampleIndexesForSpecialEvent.find(sampleIndex) != oldSampleIndexesForSpecialEvent.cend()) {
                  // Must be remapped or deleted?
                  const auto it = mapping.find(sampleIndex);
                  if (it == mapping.cend()) {
                      // Deleted.
                      return std::make_unique<SpecialCell>(SpecialCell::buildEmptySpecialCell());
                  }
                  // Remapped.
                  const auto newIndex = it->second;
                  if (newIndex.isAbsent()) {
                      return nullptr;       // Not present. Don't change it.
                  }
                  return std::make_unique<SpecialCell>(SpecialCell::buildSpecialCell(newIndex.getValue()));
              }
              // No sample found.
              return nullptr;
          });
    }

    return { cellsAndLocations, specialCellsAndLocations };
}

}   // namespace arkostracker
