#include "FrameCounter.h"

#include "../../../../song/Song.h"

namespace arkostracker 
{

std::tuple<int, int, int> FrameCounter::count(const Song& song, const Id& subsongId, const OptionalValue<Location>& upToExcludedOptional) noexcept
{
    auto frameCountTotal = 0;
    auto frameCountUpTo = 0;
    auto frameLoopTo = 0;

    auto wentPastUpTo = false;

    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto[loopPositionIndex, endPositionIndex] = subsong.getLoopStartAndEndPosition();
        const auto positionCount = subsong.getLength();

        auto upToExcluded = Location(subsongId, 0);
        if (upToExcludedOptional.isPresent()) {
            upToExcluded = upToExcludedOptional.getValue();
            jassert(upToExcluded.getSubsongId() == subsongId);         // Not the same subsong!!!
        }

        auto currentSpeed = subsong.getMetadata().getInitialSpeed();

        // Browses through all the Positions. The position count is checked in case the last position is deleted.
        for (auto positionIndex = 0; (positionIndex <= endPositionIndex) && (positionIndex < positionCount); ++positionIndex) {
            if (loopPositionIndex == positionIndex) {
                frameLoopTo = frameCountTotal;
            }

            const auto positionHeight = subsong.getPositionRef(positionIndex).getHeight();
            const auto& speedTrack = subsong.getSpecialTrackRefFromPosition(positionIndex, true);

            // If we have to count the UpTo, and we're on the same position, it means we have to count the frames of the sub-track.
            auto framesToAddToUpTo = 0;
            auto upToReachedNow = false;
            if (!wentPastUpTo && upToExcludedOptional.isPresent() && (upToExcluded.getPosition() == positionIndex)) {
                auto tempSpeed = currentSpeed;        // Hackish... But we don't want the speed modified in this pass.
                framesToAddToUpTo = countFramesInSpeedTrack(speedTrack, tempSpeed, upToExcluded.getLine());
                upToReachedNow = true;
            }

            // Counts the frames of the special track.
            const auto framesToAddToTotal = countFramesInSpeedTrack(speedTrack, currentSpeed, positionHeight);
            // If the UpTo has not been reached/past yet, use the same value as the normal pass.
            if (!wentPastUpTo && !upToReachedNow) {
                framesToAddToUpTo = framesToAddToTotal;
            }

            if (upToReachedNow) {
                wentPastUpTo = true;
            }

            frameCountTotal += framesToAddToTotal;
            frameCountUpTo += framesToAddToUpTo;
        }
    });

    // Corrects the UpTo.
    if (upToExcludedOptional.isAbsent()) {
        frameCountUpTo = frameCountTotal;
    } else if (!wentPastUpTo) {
        frameCountUpTo = frameCountTotal;
        jassertfalse;           // Target not found? Happens when playing after the end. However, this can be useful to the user.
    }
    return { frameCountTotal, frameCountUpTo, frameLoopTo };
}

int FrameCounter::countFramesInSpeedTrack(const SpecialTrack& speedTrack, int& currentSpeed, const int positionHeight) noexcept
{
    // Possible case: the UpTo is the first line.
    if (positionHeight <= 0) {
        return 0;
    }

    const auto lastLineOfPattern = positionHeight - 1;
    jassert(lastLineOfPattern >= 0);

    auto frameCount = 0;
    auto currentIndex = 0;
    for (const auto& entry : speedTrack.getNonEmptyItems()) {
        auto foundIndex = entry.first;
        auto lastIteration = false;
        // Don't go over the expected height.
        if (foundIndex > lastLineOfPattern) {
            foundIndex = lastLineOfPattern + 1;
            lastIteration = true;           // We can stop, a "height" has been reached.
        }

        jassert(foundIndex >= currentIndex);            // Vector not ordered?
        if (foundIndex > currentIndex) {
            // Counts the "space" as "time".
            frameCount += (currentSpeed * (foundIndex - currentIndex));
        }

        currentIndex = foundIndex;

        if (lastIteration) {
            break;
        }

        // Do NOT take the speed in account if it was out of bounds!
        currentSpeed = entry.second.getValue();
        jassert(currentSpeed > 0);              // This happens when encoding an empty SpecialCell. Shouldn't happen, only non-empty Cells are stored.
    }

    // Finishes the pattern height.
    if (currentIndex <= lastLineOfPattern) {
        frameCount += (currentSpeed * (lastLineOfPattern + 1 - currentIndex));
    }

    return frameCount;
}

}   // namespace arkostracker
