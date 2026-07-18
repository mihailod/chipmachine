#pragma once

#include "BaseItemOptimizer.h"
#include "../../../../song/Expression.h"

namespace arkostracker 
{

/**
 * Optimizes the given Expression.
 *
 * In order:
 * - Flattens the Expression, if wanted.
 * - Remove the cells past the end.
 * - If looping, checks that what is before the loop is not the same as the loop itself. If yes, can be removed.
 * - If the cells are all duplicates on X steps, increase the speed accordingly and remove the duplicates.
 * - Removes duplications before the loop (0, 3, 7, 0, 3, 7 -> 0, 3, 7). Or (0, 0, 0, 0 -> 0). Or (1,2,[3,4,2] -> 1,[2,3,4].
 *
 * Important: if flatten, the Expression is directly flattened, so the "flatten" flag must not be used afterwards.
 */
class ExpressionOptimizer final : BaseItemOptimizer<Expression, int>
{
public:
    /**
     * Constructor.
     * @param expression the Expression.
     * @param flatten should be true for exports. If true, the shift will be applied. Use false for non-export, but some optimizations will be skipped.
     */
    ExpressionOptimizer(Expression& expression, bool flatten) noexcept;

    /** Optimizes the Expression. It will be modified. Nothing happens if the Expression cannot be optimized. */
    void optimize() noexcept;

private:
    /** Flattens the Expression. It will remove the shift. */
    void flattenExpression() noexcept;

    // BaseItemOptimizer method implementations.
    // ============================================
    Loop getLoop() noexcept override;
    void setLoop(const Loop& loop) noexcept override;
    int getLength() noexcept override;
    int getSpeed() noexcept override;
    void setSpeed(int speed) noexcept override;
    void removeCells(int cellIndex, int cellCountToRemove) noexcept override;
    int getCell(int cellIndex) noexcept override;
    bool areCellsMusicallyEqual(const int& left, const int& right) noexcept override;

    const bool flatten;
};

}   // namespace arkostracker
