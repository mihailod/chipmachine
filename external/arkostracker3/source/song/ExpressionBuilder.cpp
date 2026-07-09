#include "ExpressionBuilder.h"

#include "../utils/NumberUtil.h"

namespace arkostracker
{

Expression ExpressionBuilder::buildArpeggio3Notes(const int effectLogicalValue) noexcept
{
    Expression inlineArpeggio(true, juce::String("Inline Arpeggio"), true);
    // No need to add the base value!
    inlineArpeggio.addValue(NumberUtil::getSecondNibble(effectLogicalValue));
    inlineArpeggio.addValue(NumberUtil::getFirstNibble(effectLogicalValue));
    inlineArpeggio.setLoopStartAndEnd(0, 2);

    return inlineArpeggio;
}

Expression ExpressionBuilder::buildArpeggio4Notes(const int effectLogicalValue) noexcept
{
    Expression inlineArpeggio(true, juce::String("Inline Arpeggio"), true);
    // No need to add the base value!
    inlineArpeggio.addValue(NumberUtil::getThirdNibble(effectLogicalValue));
    inlineArpeggio.addValue(NumberUtil::getSecondNibble(effectLogicalValue));
    inlineArpeggio.addValue(NumberUtil::getFirstNibble(effectLogicalValue));
    inlineArpeggio.setLoopStartAndEnd(0, 3);

    return inlineArpeggio;
}

}   // namespace arkostracker
