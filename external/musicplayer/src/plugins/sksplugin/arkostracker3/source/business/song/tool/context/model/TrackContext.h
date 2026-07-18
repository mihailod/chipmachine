#pragma once

#include "LineContext.h"

namespace arkostracker
{

/** Represents a Context in the lines of a "track". */
class TrackContext
{
public:
    TrackContext() noexcept;

    /**
     * Sets the context on the given line of the track. It should not be already filled (asserts).
     * @param lineIndex the line index.
     * @param lineContext the line context.
     */
    void setContext(int lineIndex, const LineContext& lineContext) noexcept;

    /**
     * @return the line context for the given line. If no data, a default value is returned.
     * @param lineIndex the line index.
     */
    LineContext getContext(int lineIndex) const noexcept;

    /** Sets the initial volume slide of the track. */
    void setInitialVolumeSlide(FpFloat volumeSlide) noexcept;
    /** @return the volume slide at the beginning of the track. */
    FpFloat getInitialVolumeSlide() const noexcept;

    /** Sets the initial speed of the track. */
    void setInitialSpeed(int speed) noexcept;
    /** @return the initial speed of the track. */
    int getInitialSpeed() const noexcept;

private:
    FpFloat initialVolumeSlide;                         // The volume slide at the beginning of the track.
    int initialSpeed;                                   // The speed at the beginning of the track.
    std::map<int, LineContext> lineIndexToContext;
};

}   // namespace arkostracker
