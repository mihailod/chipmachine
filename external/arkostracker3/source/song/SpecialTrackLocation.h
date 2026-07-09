#pragma once

namespace arkostracker
{

/** A SpecialTrack Location in a Subsong. */
class SpecialTrackLocation
{
public:
    /**
     * Constructor.
     * @param isSpeedTrack true if Speed Track, false for Event.
     * @param positionIndex the position index. Must be valid.
     */
    SpecialTrackLocation(bool isSpeedTrack, int positionIndex) noexcept;

    int getPositionIndex() const noexcept;
    bool isSpeedTrack() const noexcept;

    bool operator==(const SpecialTrackLocation& rhs) const;
    bool operator!=(const SpecialTrackLocation& rhs) const;

private:
    bool speedTrack;
    int positionIndex;
};

}   // namespace arkostracker