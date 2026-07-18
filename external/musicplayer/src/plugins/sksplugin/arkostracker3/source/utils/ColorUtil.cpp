#include "ColorUtil.h"

namespace arkostracker
{

juce::uint32 ColorUtil::setAlphaToFull(const juce::uint32 color) noexcept
{
    return color | 0xFF000000U;
}

}       // namespace arkostracker