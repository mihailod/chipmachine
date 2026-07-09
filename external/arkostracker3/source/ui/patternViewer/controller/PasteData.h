#pragma once

#include <vector>

#include "../../../song/cells/Cell.h"
#include "../../../song/cells/SpecialCell.h"
#include "CaptureFlags.h"

namespace arkostracker 
{

/** Simple holder of the data needed to perform a Paste. */
class PasteData
{
public:
    /** Constructor for invalid data. */
    PasteData();

    /**
     * Constructor.
     * @param height the height. It will define how many cells are copied, regardless of how they many they actually have.
     * @param tracksToCells the cells, for each Tracks.
     * @param captureFlags the Capture Flags, for each Track. Must be the same amount!
     * @param speedCells the Speed Cells, or empty if there is no Speed Track to paste.
     * @param eventCells the Event Cells, or empty if there is no Event Track to paste.
     */
    PasteData(int height, std::vector<std::vector<Cell>> tracksToCells, std::vector<CaptureFlags> captureFlags, std::vector<SpecialCell> speedCells,
              std::vector<SpecialCell> eventCells);

    /** @return true if the data is valid. */
    bool isValid() const noexcept;

    const int height;
    const std::vector<std::vector<Cell>> tracksToCells;
    const std::vector<CaptureFlags> captureFlags;
    const std::vector<SpecialCell> speedCells;
    const std::vector<SpecialCell> eventCells;
};

}   // namespace arkostracker
