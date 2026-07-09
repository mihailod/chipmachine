#include "MergeSongProcess.h"

#include "../../../song/cells/CellConstants.h"
#include "browser/CellBrowser.h"
#include "optimizers/SongOptimizer.h"

namespace arkostracker
{

std::unique_ptr<Song> MergeSongProcess::merge(const Song& originalSong, const Song& songToMerge) noexcept
{
    auto currentSong = originalSong;        // Performs a copy.
    const auto currentSongInstrumentCount = currentSong.getInstrumentCount();
    const auto currentSongArpeggioCount = currentSong.getConstExpressionHandler(true).getCount();
    const auto currentSongPitchCount = currentSong.getConstExpressionHandler(false).getCount();

    // Gets the Subsongs.
    const auto additionalSubsongIds = songToMerge.getSubsongIds();

    // Extracts the Subsongs.
    for (const auto& subsongId : additionalSubsongIds) {
        songToMerge.performOnConstSubsong(subsongId, [&] (const Subsong& subsongToMerge) {
            auto newSubsong = std::make_unique<Subsong>(subsongToMerge);
            currentSong.addSubsong(std::move(newSubsong));
        });
    }

    // Adds the Instruments.
    songToMerge.performOnConstInstruments([&](const std::vector<std::unique_ptr<Instrument>>& instrumentsToMerge) {
        auto instrumentIndex = 0;
        for (const auto& instrumentToMerge : instrumentsToMerge) {
            if (instrumentIndex > 0) {  // Skips the 0th.
                auto newInstrument = std::make_unique<Instrument>(*instrumentToMerge);
                currentSong.addInstrument(std::move(newInstrument));
            }
            ++instrumentIndex;
        }
    });

    // Adds the Expressions.
    addExpressions(true, currentSong, songToMerge);
    addExpressions(false, currentSong, songToMerge);

    // Remaps the instruments and expressions in all the tracks. They must be shifted.
    const CellBrowser cellBrowser(currentSong);
    for (const auto& subsongId : additionalSubsongIds) {
        cellBrowser.browseCells(false, subsongId, [&] (const Cell& cell) {
            auto modified = false;

            // Shifts the instruments.
            auto newCell = std::make_unique<Cell>(cell);
            if (cell.isInstrument() && !cell.isRst()) {
                const auto newInstrumentIndex = cell.getInstrument().getValue() + currentSongInstrumentCount - 1;    // Excluding the 0th.
                newCell->setInstrument(newInstrumentIndex);
                modified = true;
            }

            // The same with the expressions.
            const auto& effects = cell.getEffects();
            modified |= shiftExpressions(effects, Effect::arpeggioTable, currentSongArpeggioCount, *newCell);
            modified |= shiftExpressions(effects, Effect::pitchTable, currentSongPitchCount, *newCell);

            return modified ? std::move(newCell) : nullptr;
        });
    }

    // Optimizes the Song, creating a new one. What is optimized is only the instruments and expressions (to avoid duplications).
    auto optimizedSong = SongOptimizer::optimize(currentSong, false, false,
        true, true,
        false, false,
        true,
        false, false, false,
        currentSong.getSubsongIds());

    // Too many items?
    constexpr auto maximumItemCount = 0x100;
    if ((optimizedSong->getInstrumentCount() > maximumItemCount)
        || (optimizedSong->getExpressionHandler(true).getCount() > maximumItemCount)
        || (optimizedSong->getExpressionHandler(false).getCount() > maximumItemCount)) {
        return nullptr;
    }

    return optimizedSong;
}

void MergeSongProcess::addExpressions(const bool isArpeggio, Song& currentSong, const Song& songToMerge) noexcept
{
    songToMerge.getConstExpressionHandler(isArpeggio).performOnConstExpressions([&] (const std::vector<std::unique_ptr<Expression>>& expressions) {
        auto expressionIndex = 0;
        for (const auto& expression : expressions) {
            if (expressionIndex > 0) {  // Skips the 0th.
                currentSong.getExpressionHandler(isArpeggio).addExpression(*expression);
            }
            ++expressionIndex;
        }
  });
}

bool MergeSongProcess::shiftExpressions(const CellEffects& cellEffects, const Effect effectToSearch, const int expressionCount, Cell& cell) noexcept
{
    auto modified = false;
    for (auto effectIndex = 0; effectIndex < CellConstants::effectCount; ++effectIndex) {
        if (const auto effect = cellEffects.getEffect(effectIndex); (effect.getEffect() == effectToSearch)) {
            if (const auto expressionIndex = effect.getEffectLogicalValue(); expressionIndex > 0) {
                const auto newCellEffect = CellEffect::buildFromLogicalValue(effectToSearch, expressionIndex + expressionCount - 1);        // Excluding the 0th.
                cell.setEffect(effectIndex, newCellEffect);
                modified = true;
            }
        }
    }

    return modified;
}

}   // namespace arkostracker
