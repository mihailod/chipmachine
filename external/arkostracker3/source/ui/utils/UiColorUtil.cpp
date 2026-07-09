#include "UiColorUtil.h"

namespace arkostracker
{

OptionalValue<juce::uint32> UiColorUtil::toUInt32(const OptionalValue<juce::Colour>& color) noexcept
{
    if (color.isAbsent()) {
        return { };
    }

    return { color.getValueRef().getARGB() };
}

}       // namespace arkostracker