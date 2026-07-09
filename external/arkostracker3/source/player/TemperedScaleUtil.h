#pragma once

namespace arkostracker
{

/**
 * Calculates frequencies in Hz, in an equal-tempered scale.
 * Reference: http://subsynth.sourceforge.net/midinote2freq.html
 */
class TemperedScaleUtil         // TODO TU this.
{
public:
    /** Prevents construction. */
    TemperedScaleUtil() = delete;

    /**
        Returns the frequency in Hz of the given note and octave.
        @param noteInOctave the note, from 0 to 11 included.
        @param octave the octave, from 0 included.
        @param referenceFrequency the reference frequency, in Hz (440 for example).
    */
    static float getFrequency(int noteInOctave, int octave, float referenceFrequency) noexcept;

    /**
     * @return the frequency in Hz of the given note number.
     * @param note the note, from 0.
     * @param referenceFrequency the reference frequency, in Hz (440 for example).
     * @param noteReference 9 "normally", but sample player may want another value, such as 12, to emulate what other trackers do.
     */
    static float getFrequency(int note, float referenceFrequency, int noteReference = 9) noexcept;

    /**
     * @return the frequency in Hz of the given note number, dedicated to the samples.
     * This is because, for samples, we want C-5 to be the reference 440, so that this plays the sample
     * "as it is". Else, it will be three semi-tones lower than what other "tracker" do.
     * @param note the note, from 0.
     * @param referenceFrequency the reference frequency, in Hz (440 for example).
     */
    static float getSampleFrequency(int note, float referenceFrequency) noexcept;
};

}   // namespace arkostracker
