#pragma once

namespace arkostracker
{

class Id;

/** Observer for objects wanting to be notified when Expressions change. */
class ExpressionChangeObserver
{
public:
    enum class What: unsigned char
    {
        name = 1U << 0U,
        /** Loop start and/or end. */
        loop = 1U << 1U,
        speed = 1U << 2U,
        shift = 1U << 3U,
        /** The data itself. */
        data = 1U << 4U
    };

    /** Destructor. */
    virtual ~ExpressionChangeObserver() = default;

    /**
     * Called when an Expression has changed. It does not include the Expression being deleted.
     * @param expressionId the id of the Expression.
     * @param whatChanged the bits of what has changed.
     */
    virtual void onExpressionChanged(const Id& expressionId, unsigned int whatChanged) = 0;

    /**
     * Called when an Expression cells has changed.
     * @param expressionId the id of the Expression.
     * @param cellIndex what has changed.
     * @param mustAlsoRefreshPastIndex true if the cells after the index must also be refreshed (because added, removed, etc.).
     */
    virtual void onExpressionCellChanged(const Id& expressionId, int cellIndex, bool mustAlsoRefreshPastIndex) = 0;

    /** Called when multiple Expressions have changed (moved, deleted, etc.). */
    virtual void onExpressionsInvalidated() = 0;
};

}   // namespace arkostracker
