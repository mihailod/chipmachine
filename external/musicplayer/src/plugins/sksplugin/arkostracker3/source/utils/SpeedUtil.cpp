#include "SpeedUtil.h"

#include "NumberUtil.h"
#include <cmath>

namespace arkostracker 
{

int SpeedUtil::convertBpmToSpeed(const int bpm, const float linesPerBeat, const float replayFrequencyHz) noexcept
{
    // See https://modarchive.org/forums/index.php?topic=2709.0
    // BPM = 60 / ((speed * linesPerBeat) / replayRateHz) // 60 because 60 seconds per minute.
    // Example: BPM = 60 / ((6 * 4) / 50) = 125

    // So...
    // Speed = (60 * replayRateHz) / (BPM * linesPerBeat).
    // For midi:
    // The line per beat is 4 normally, but the tempo must also take in account the original ppq and the chosen ppq.
    // So the line per beat is directly dependent on them: with a "conventional" ppq of originalPpq / 4, we have a ratio of 4, thus 4 lines per beat.

    const auto speedDouble = ((60.0 * replayFrequencyHz) / (static_cast<float>(bpm) * linesPerBeat));
    const auto speed = static_cast<int>(std::round(speedDouble));

    // Security, just in case.
    return NumberUtil::correctNumber(speed, 1, 255);
}

}   // namespace arkostracker
