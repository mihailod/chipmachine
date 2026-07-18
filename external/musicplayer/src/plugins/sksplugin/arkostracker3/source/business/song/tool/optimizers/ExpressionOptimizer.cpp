#include "ExpressionOptimizer.h"

namespace arkostracker 
{

ExpressionOptimizer::ExpressionOptimizer(Expression& pExpression, const bool pFlatten) noexcept :
        BaseItemOptimizer(pExpression),
        flatten(pFlatten)
{
}

void ExpressionOptimizer::optimize() noexcept
{
    if (flatten) {
        flattenExpression();
    }

    removePastEndCells();
    // This would be too unlikely to make it work without flattening.
    if (flatten) {
        //removePreLoopDuplications();
        removeCycles();
        removeDuplicateCells();
        // TODO Could replace the "pre loop duplications" above?
        optimizePreLoop();          // TODO Also in Instruments.
    }
}

void ExpressionOptimizer::flattenExpression() noexcept
{
    auto& item = getItem();

    const auto shift = item.getShift();
    if (shift <= 0) {
        return;
    }
    item.setShift(0);

    // Inserts the values.
    for (auto cellIndex = 0; cellIndex < shift; ++cellIndex) {
        item.insertValue(0, 0);
    }

    // Updates the loop.
    const auto& originalLoop = item.getLoop();
    item.setLoop(Loop(originalLoop.getStartIndex() + shift, originalLoop.getEndIndex() + shift, true));
}

// BaseItemOptimizer method implementations.
// ============================================

Loop ExpressionOptimizer::getLoop() noexcept
{
    return getItem().getLoop();
}

void ExpressionOptimizer::setLoop(const Loop& loop) noexcept
{
    getItem().setLoop(loop);
}

int ExpressionOptimizer::getLength() noexcept
{
    return getItem().getLength();
}

int ExpressionOptimizer::getSpeed() noexcept
{
    return getItem().getSpeed();
}

void ExpressionOptimizer::setSpeed(const int speed) noexcept
{
    getItem().setSpeed(speed);
}

void ExpressionOptimizer::removeCells(const int cellIndex, const int cellCountToRemove) noexcept
{
    getItem().removeValues(cellIndex, cellCountToRemove);
}

int ExpressionOptimizer::getCell(const int cellIndex) noexcept
{
    return getItem().getValue(cellIndex);
}

bool ExpressionOptimizer::areCellsMusicallyEqual(const int& left, const int& right) noexcept
{
    return (left == right);
}

}   // namespace arkostracker
