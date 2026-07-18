#include "ChangeExpressionsOrdering.h"

#include "../../../song/Song.h"
#include "browser/CellBrowser.h"

namespace arkostracker 
{

std::vector<CellAndLocation> ChangeExpressionsOrdering::changeExpressionsInSong(const bool isArpeggio, Song& song, const std::unordered_map<int, OptionalInt>& mapping) noexcept
{
    const auto targetEffect = isArpeggio ? Effect::arpeggioTable : Effect::pitchTable;

    const CellBrowser cellBrowser(song);

    // Browses all the Cells, changes the ones that needs to. Returns the original ones.
    return cellBrowser.browseCells(true, [&] (const Cell& cell) -> std::unique_ptr<Cell> {
        // Any effect that matches?
        if (!cell.hasEffects()) {
            return nullptr;
        }
        auto cellEffects = cell.getEffects();

        // Modifies each effect(s) that is the one we look for.
        auto modified = false;
        const auto indexToEffect = cellEffects.getExistingEffects();
        for (const auto& entry : indexToEffect) {
            const auto effectIndex = entry.first;
            const auto& cellEffect = entry.second;
            if (cellEffect.getEffect() == targetEffect) {
                const auto effectValue = cellEffect.getEffectLogicalValue();

                // The effect must be remapped or deleted. If not present, don't change it.
                if (auto iterator = mapping.find(effectValue); iterator != mapping.cend()) {
                    if (const auto newValue = iterator->second; newValue.isAbsent()) {
                        // Absent value, so must be deleted.
                        cellEffects.clearEffect(effectIndex);
                        modified = true;
                    } else if (newValue.getValue() != effectValue) {
                        // New mapping.
                        cellEffects.setEffect(effectIndex, CellEffect::buildFromLogicalValue(targetEffect, newValue.getValue()));
                        modified = true;
                    }
                }
            }
        }

        if (!modified) {
            return nullptr;
        }

        // Returns a modified Cell.
        auto newCell = std::make_unique<Cell>(cell);
        newCell->setEffects(cellEffects);

        return newCell;
    });
}


}   // namespace arkostracker

