#include "BpmCalculator.h"

#include "../../../actions/sfx/SetExportedSoundEffects.h"

namespace arkostracker
{

int BpmCalculator::calculateBpm(const int rowsPerBeat, const int speed, const float replayFrequencyHz) noexcept
{
    // With the help of https://modarchive.org/forums/index.php?topic=2709.0.
    const auto bpm = 60.0F / (static_cast<float>(speed * rowsPerBeat) * (1.0F / replayFrequencyHz));

    return static_cast<int>(std::lround(bpm));
}

}   // namespace arkostracker
