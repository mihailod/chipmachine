#pragma once

#include "../patternViewer/CellsModifier.h"

namespace arkostracker
{

/** Remaps instrument indexes. */
class RemapInstruments final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param selectedData info about the data to process.
     * @param sourceInstrumentIndexes the instrument indexes to map into the target instrument index.
     * @param targetInstrumentIndex the target instrument index.
     */
    RemapInstruments(SongController& songController, SelectedData selectedData, std::unordered_set<int> sourceInstrumentIndexes, int targetInstrumentIndex) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;

private:
    const std::unordered_set<int> sourceInstrumentIndexes;
    const int targetInstrumentIndex;
};

} // namespace arkostracker
