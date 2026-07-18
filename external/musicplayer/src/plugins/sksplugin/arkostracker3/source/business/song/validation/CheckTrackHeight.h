#pragma once

namespace arkostracker 
{

/** Checks the height of a track. */
class CheckTrackHeight
{
public:
    /**
     * @return the corrected track height.
     * @param trackHeight the height to correct.
     */
    static int correctHeight(int trackHeight) noexcept;
};


}   // namespace arkostracker

