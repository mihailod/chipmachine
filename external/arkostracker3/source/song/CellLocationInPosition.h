#pragma once

#include "../utils/Id.h"

namespace arkostracker
{

/** The location of a Cell, in a Position on a Subsong. */
class CellLocationInPosition
{
public:
    /** Default constructor, do not use. Only for templating. */
    CellLocationInPosition() noexcept;

    /**
     * Constructor.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positionIndex the position index. Must be valid.
     * @param lineIndex the line index in the Position. Must be valid.
     * @param channelIndex the channel index. Must be valid.
     */
    CellLocationInPosition(Id subsongId, int positionIndex, int lineIndex, int channelIndex) noexcept;

    Id getSubsongId() const noexcept;
    int getPositionIndex() const noexcept;
    int getLineIndex() const noexcept;
    int getChannelIndex() const noexcept;

    bool operator==(const CellLocationInPosition& other) const;
    bool operator!=(const CellLocationInPosition& other) const;

private:
    Id subsongId;
    int positionIndex;
    int lineIndex;
    int channelIndex;
};

}   // namespace arkostracker
