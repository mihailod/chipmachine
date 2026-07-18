#pragma once

namespace arkostracker 
{

class Expression;

/** Corrects the given Expression. */
class CorrectExpression
{
public:
    /**
     * Corrects the given expression.
     * @param expression the Expression. Will be modified if needed.
     */
    void correctExpression(Expression& expression) noexcept;
};

}   // namespace arkostracker

