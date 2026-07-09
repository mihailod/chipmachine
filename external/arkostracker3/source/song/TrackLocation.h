#pragma once

namespace arkostracker
{

/** A Track Location in a Subsong. */
class TrackLocation
{
public:
    /**
     * Constructor.
     * @param positionIndex the position index. Must be valid.
     * @param channelIndex the channel index. Must be valid.
     */
    TrackLocation(int positionIndex, int channelIndex) noexcept;

    int getPositionIndex() const noexcept;
    int getChannelIndex() const noexcept;

    bool operator==(const TrackLocation& rhs) const;
    bool operator!=(const TrackLocation& rhs) const;

private:
    int positionIndex;
    int channelIndex;
};

}   // namespace arkostracker
