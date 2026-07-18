#include "DetermineSpeed.h"

#include "../../../../song/Song.h"

namespace arkostracker 
{

int DetermineSpeed::determineSpeed(const Song& song, const Location& location) noexcept
{
    auto currentSpeed = 6;       // Set in case the Subsong doesn't exist.

    const auto positionToReach = location.getPosition();
    const auto lineToReach = location.getLine();

    song.performOnConstSubsong(location.getSubsongId(), [&](const Subsong& subsong) noexcept {
        currentSpeed = subsong.getMetadata().getInitialSpeed();
        const auto length = subsong.getLength();
        // Browses each position, up to the position to reach.
        for (auto currentPositionIndex = 0; currentPositionIndex <= positionToReach; ++currentPositionIndex) {
            if (currentPositionIndex >= length) {
                jassertfalse;           // The destination is out of bounds! We keep the latest speed anyway.
                break;
            }

            // If we have reached the target position, we need to get the speed the closest to the target line.
            // If we are in any previous positions, we need to get the speed at the bottom of the position.
            auto currentLineToReach = lineToReach;
            if (currentPositionIndex != positionToReach) {
                const auto patternHeight = subsong.getPositionHeight(currentPositionIndex);
                currentLineToReach = (patternHeight - 1);
            }
            const auto& speedTrack = subsong.getSpecialTrackRefFromPosition(currentPositionIndex, true);

            const auto specialCell = speedTrack.getLatestCell(currentLineToReach);
            if (!specialCell.isEmpty()) {
                currentSpeed = specialCell.getValue();
            }
        }
    });

    return currentSpeed;
}

}   // namespace arkostracker
