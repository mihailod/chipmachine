#pragma once

#include "Expression.h"

namespace arkostracker
{

class ExpressionBuilder
{
public:
    /** Prevents instantiation. */
    ExpressionBuilder() = delete;

    /**
     * @return an Arpeggio 3 notes.
     * @param effectLogicalValue the logical value of the effect.
     */
    static Expression buildArpeggio3Notes(int effectLogicalValue) noexcept;

    /**
     * @return an Arpeggio 4 notes.
     * @param effectLogicalValue the logical value of the effect.
     */
    static Expression buildArpeggio4Notes(int effectLogicalValue) noexcept;
};

}   // namespace arkostracker
