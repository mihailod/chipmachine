#pragma once

namespace arkostracker
{

class BpmCalculator
{
public:
    /**
     * @return a BPM according to the given data.
     * @param rowsPerBeat how many rows is considered a beat (typically, highlight).
     * @param speed the speed (6 for example).
     * @param replayFrequencyHz the replay frequency (50Hz, etc.).
     */
    static int calculateBpm(int rowsPerBeat, int speed, float replayFrequencyHz) noexcept;
};

}   // namespace arkostracker
