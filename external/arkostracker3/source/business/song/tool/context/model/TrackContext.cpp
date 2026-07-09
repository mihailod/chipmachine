#include "TrackContext.h"

#include "../../../../../song/subsong/SubsongConstants.h"
#include "../../../../../song/tracks/TrackConstants.h"

namespace arkostracker
{

TrackContext::TrackContext() noexcept :
        initialVolumeSlide(),
        initialSpeed(SubsongConstants::defaultSpeed),
        lineIndexToContext()
{
}

void TrackContext::setContext(const int lineIndex, const LineContext& lineContext) noexcept
{
    jassert(lineIndexToContext.find(lineIndex) == lineIndexToContext.cend());       // Shouldn't be there already!

    lineIndexToContext.insert({ lineIndex, lineContext });
}

LineContext TrackContext::getContext(const int lineIndex) const noexcept
{
    if ((lineIndex < 0) || (lineIndex > TrackConstants::lastPossibleIndex)) {
        jassertfalse;
        return { };
    }

    // Any matching key?
    if (const auto it = lineIndexToContext.find(lineIndex); it != lineIndexToContext.cend()) {
        return it->second;
    }
    // Finds the first item that is "equal or less than the key". Frigging "lower_bound" cannot accept decreasing, maybe it useless.
    const auto it = std::find_if(lineIndexToContext.crbegin(), lineIndexToContext.crend(), [&] (const auto& pair) { // NOLINT(*-use-ranges)
        return (pair.first <= lineIndex);
    });
    if (it != lineIndexToContext.crend()) {
        return it->second;
    }

    return { };
}

FpFloat TrackContext::getInitialVolumeSlide() const noexcept
{
    return initialVolumeSlide;
}

void TrackContext::setInitialVolumeSlide(const FpFloat volumeSlide) noexcept
{
    initialVolumeSlide = volumeSlide;
}

void TrackContext::setInitialSpeed(const int speed) noexcept
{
    initialSpeed = speed;
}

int TrackContext::getInitialSpeed() const noexcept
{
    return initialSpeed;
}

}   // namespace arkostracker
