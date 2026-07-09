#pragma once

namespace arkostracker 
{

/** Utility class about speed/bpm. */
class SpeedUtil
{
public:
    /** Prevents instantiation. */
    SpeedUtil() = delete;

    /**
     * Converts the given BPM to a speed.
     * @param bpm the BPM.
     * @param linesPerBeat how many lines per beat. Float because helps get a better accuracy for MIDI, for example.
     * @param replayFrequencyHz the replay frequency (50hz for example).
     * @return the speed (>0).
     */
    static int convertBpmToSpeed(int bpm, float linesPerBeat, float replayFrequencyHz) noexcept;
};

}   // namespace arkostracker
