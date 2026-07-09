#pragma once

#include "CellsModifier.h"

namespace arkostracker
{

/** Transpose only notes. */
class TransposeNote final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param transposeRate how much to transpose.
     * @param selectedData info about the data to process.
     * @param instrumentIndexes if empty, all instruments are transposed (except RST). If present, only these are transposed (except RST).
     */
    TransposeNote(SongController& songController, int transposeRate, SelectedData selectedData, std::unordered_set<int> instrumentIndexes = { }) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;

private:
    const int transposeRate;
    std::unordered_set<int> instrumentIndexesToTranspose;           // Or empty.
};

} // namespace arkostracker
