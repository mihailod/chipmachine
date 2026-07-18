#include "CorrectExpression.h"

#include "../../../song/Expression.h"
#include "../../../song/ExpressionConstants.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

void CorrectExpression::correctExpression(Expression& expression) noexcept       // NOLINT(readability-convert-member-functions-to-static)
{
    const auto isArpeggio = expression.isArpeggio();
    const auto minValue = isArpeggio ? ExpressionConstants::minimumArpeggio : ExpressionConstants::minimumPitch;
    const auto maxValue = isArpeggio ? ExpressionConstants::maximumArpeggio : ExpressionConstants::maximumPitch;

    // Checks the length. There must be at least one item. If not, creates it.
    auto length = expression.getLength();
    if (length == 0) {
        expression.addValue(0);

        length = expression.getLength();
        jassert(length == 1);
    }

    // Checks the loop bounds.
    auto end = expression.getEnd();
    if ((end < 0) || (end >= length)) {
        end = length - 1;
    }

    auto start = expression.getLoopStart();
    if ((start < 0) || (start > end)) {
        start = 0;
    }
    expression.setLoopStartAndEnd(start, end);

    // Checks the speed.
    const auto speed = NumberUtil::correctNumber(expression.getSpeed(), ExpressionConstants::minimumSpeed, ExpressionConstants::maximumSpeed);
    expression.setSpeed(speed);

    // Checks the values.
    for (auto index = 0; index < length; ++index) {
        const auto value = expression.getValue(index);
        if ((value < minValue) || (value > maxValue)) {
            const auto correctedValue = NumberUtil::correctNumber(value, minValue, maxValue);
            expression.setValue(index, correctedValue);
        }
    }
}


}   // namespace arkostracker

