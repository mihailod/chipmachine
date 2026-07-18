#pragma once

#include "../patternViewer/CellsModifier.h"

namespace arkostracker
{

/** Swaps instrument indexes. */
class SwapInstruments final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param selectedData info about the data to process.
     * @param firstInstrumentIndex one instrument index. Must not be RST.
     * @param secondInstrumentIndex one instrument index. Must not be RST.
     */
    SwapInstruments(SongController& songController, SelectedData selectedData, int firstInstrumentIndex, int secondInstrumentIndex) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;

private:
    const int firstInstrumentIndex;
    const int secondInstrumentIndex;
};

} // namespace arkostracker
