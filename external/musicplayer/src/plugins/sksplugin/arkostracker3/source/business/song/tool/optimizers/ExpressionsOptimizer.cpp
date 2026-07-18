#include "ExpressionsOptimizer.h"

#include "../CellOperations.h"
#include "../ChangeExpressionsOrdering.h"
#include "../browser/CellBrowser.h"
#include "ExpressionOptimizer.h"

namespace arkostracker 
{

ExpressionsOptimizer::ExpressionsOptimizer(const Song& pOriginalSong, const bool pFlatten) noexcept :
        originalSong(pOriginalSong),
        newSong(std::make_unique<Song>(pOriginalSong)),
        flatten(pFlatten)
{
}

std::unique_ptr<Song> ExpressionsOptimizer::optimize() noexcept
{
    jassert(newSong != nullptr);        // Called twice?

    optimize(true);
    optimize(false);

    return std::move(newSong);
}

void ExpressionsOptimizer::optimize(const bool isArpeggio) noexcept
{
    // Makes sure the illegal Expressions are converted to 0.
    checkExpressionsValidity(true);
    checkExpressionsValidity(false);

    constexpr auto stopAtLoopEnd = false;
    const auto effectToSearch = isArpeggio ? Effect::arpeggioTable : Effect::pitchTable;

    auto& songExpressionHandler = newSong->getExpressionHandler(isArpeggio);

    // Deletes all the Expressions of the new Song!
    songExpressionHandler.performOnExpressions([&](std::vector<std::unique_ptr<Expression>>& expressions) noexcept {
        expressions.clear();
    });

    std::unordered_map<int, OptionalInt> oldExpressionIndexToNew;    // If the old index is marked as Absent, it means the Expression is deleted.
    oldExpressionIndexToNew.insert({ 0, 0 });                   // Expression 0 is always present!

    std::set<int> oldExpressionIndexUsed;                       // They may not exist!
    oldExpressionIndexUsed.insert(0);                         // Expression 0 is always present.

    // Finds all the Expressions that are used.
    CellBrowser::browseConstSongCells(*newSong, stopAtLoopEnd, [&](const Cell& cell) {
        if (const auto foundEffect = cell.find(effectToSearch); foundEffect.isPresent()) {
            const auto expressionIndex = foundEffect.getValueRef().getEffectLogicalValue();
            oldExpressionIndexUsed.insert(expressionIndex);
        }

        return false;
    });

    std::vector<std::unique_ptr<Expression>> newExpressions;

    // Browses each Expression. Also makes sure each browsed Expression is not a duplicate of a previously inserted one.
    for (auto oldExpressionIndex : oldExpressionIndexUsed) {
        originalSong.getConstExpressionHandler(isArpeggio).performOnConstExpressionFromIndex(oldExpressionIndex,
            [&](const Expression& originalExpression) noexcept {
            auto newExpressionIndex = static_cast<int>(newExpressions.size());
            auto newExpression = std::make_unique<Expression>(originalExpression);
            // Before inserting the Expression, optimizes it.
            ExpressionOptimizer optimizer(*newExpression, flatten);
            optimizer.optimize();

            // Then makes sure there is no duplicate before inserting it.
            auto foundDuplicate = false;
            auto alreadyAddedExpressionIndex = 0;
            for (const auto& alreadyAddedExpression : newExpressions) {
                if (newExpression->isMusicallyEqualTo(*alreadyAddedExpression)) {
                    foundDuplicate = true;
                    break;
                }
                ++alreadyAddedExpressionIndex;
            }

            // Inserts it and fills the oldToNew map.
            if (!foundDuplicate) {
                newExpressions.push_back(std::move(newExpression));
            } else {
                // If duplicate, the "new expression" index is the one that matches.
                newExpressionIndex = alreadyAddedExpressionIndex;
            }
            // In all cases, links the old Expression index either to the new one, or the one that matches.
            oldExpressionIndexToNew.insert({ oldExpressionIndex, newExpressionIndex });
        });
    }

    // Puts the new Expressions in the new Song.
    songExpressionHandler.performOnExpressions([&] (std::vector<std::unique_ptr<Expression>>& expressions) {
        for (auto& expression : newExpressions) {
            expressions.push_back(std::move(expression));
        }
    });

    // Remaps.
    ChangeExpressionsOrdering::changeExpressionsInSong(isArpeggio, *newSong, oldExpressionIndexToNew);
}

void ExpressionsOptimizer::checkExpressionsValidity(const bool isArpeggio) noexcept
{
    // The count is done on the *original* song, but the modification is done on the Tracks of the new Song.
    const auto expressionHandler = originalSong.getConstExpressionHandler(isArpeggio);
    const auto expressionCount = expressionHandler.getCount();

    const CellBrowser cellBrowser(*newSong);
    cellBrowser.browseCells(false, IllegalExpressionTo0InCell(isArpeggio, expressionCount));
}

}   // namespace arkostracker
