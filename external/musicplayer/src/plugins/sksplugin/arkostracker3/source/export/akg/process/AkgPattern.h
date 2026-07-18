#pragma once

#include <vector>

#include "AkgPatternItem.h"

namespace arkostracker 
{

class Pattern;
class Position;

/** An "AT2" Pattern-like, which contains the height and transpositions. */     // TODO Should probably be generic to all export.
class AkgPattern
{
public:
    /**
     * Constructor.
     * @param position the Position.
     * @param pattern the Pattern the Position is linked to.
     */
    explicit AkgPattern(const Position& position, const Pattern& pattern) noexcept;

    /** Equality operator. */
    bool operator==(const AkgPattern&) const;

    /** @return the height of the pattern (>0). */
    int getHeight() const noexcept;

    /** @return the index of the SpeedTrack. No boundary is checked. */
    int getSpeedTrackIndex() const noexcept;

    /** @return the index of the EventTrack. No boundary is checked. */
    int getEventTrackIndex() const noexcept;

    /** @return how many Channels are inside this Pattern (not counting the Special Tracks). */
    int getChannelCount() const noexcept;

    /**
     * @return the transposition of a Track. No boundary is checked.
     * @param channelIndex the index of the channel (0-2 for PSG1 etc.).
     */
    int getTransposition(int channelIndex) const noexcept;

private:
    int height;                                     // The current height of the Pattern (>0).
    std::vector<AkgPatternItem> trackItems;         // The index and transposition of the Tracks for each channel and PSG.
    int speedTrackIndex;                            // The index of the Speed Track (>=0).
    int eventTrackIndex;                            // The index of the Event Track (>=0).
};

}   // namespace arkostracker
