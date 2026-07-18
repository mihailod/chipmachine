#pragma once

#include <memory>

#include "../business/model/Loop.h"
#include "../song/instrument/sample/Sample.h"

namespace arkostracker 
{

/** Representation on how to play a sample in a stream. */
class SamplePlayInfo
{
public:
    /** Constructor of an empty data (no sample to play, don't cut the possible sample). */
    SamplePlayInfo() noexcept;

    /** Builder for a SamplePlayInfo that returns no sample (cut sound). */
    static SamplePlayInfo buildNoSample() noexcept;

    /**
     * Constructor.
     * @param sample the possible sample to play.
     * @param playSampleFromStart true to play the sample from start (new sample, or triggered again).
     * @param amplification 1.0 is normal.
     * @param loop the loop.
     * @param sampleFrequencyHz the base frequency of the sample, in Hz.
     * @param pitchHz the pitch in hz to play. The higher, the higher the note (contrary to a PSG period).
     * @param referenceFrequencyHz the reference frequency, in hz (440 for example).
     * @param volume4bits the volume, from 0 to 15.
     * @param highPriority used by digidrums in an Event Track, for example. Will be heard over any PSG or other sample. But should not loop (asserts).
     * @param forceSampleFrequencyForReplay slightly hackish way to indicate the player the sample frequency must not compensated by the sample player frequency.
     * Should be false most of the time. True for special cases, such as StreamedMusicAnalyzed.
     */
    SamplePlayInfo(std::shared_ptr<Sample> sample, bool playSampleFromStart, float amplification,
                   Loop loop, int sampleFrequencyHz, float pitchHz, float referenceFrequencyHz, int volume4bits,
                   bool highPriority, bool forceSampleFrequencyForReplay = false) noexcept;

    /** @return true if there is no data at all. */
    bool isEmpty() const;

    /** @return the possible sample. */
    std::shared_ptr<Sample> getSample() const noexcept;
    /** If there is a sample, true to play the sample from start (new sample, or triggered again). */
    bool isPlaySampleFromStart() const noexcept;

    /** @return true if there is a sample. */
    bool isSamplePresent() const noexcept;

    /** @return 1.0 is normal. */
    float getAmplification() const noexcept;

    /** @return the loop. */
    const Loop& getLoop() const noexcept;

    /** @return the base frequency of the sample, in Hz. */
    int getSampleFrequencyHz() const noexcept;

    /** @return the pitch in Hz to play. The higher, the higher the note (contrary to a PSG period). */
    float getPitchHz() const noexcept;

    /** @return the volume, from 0 to 15. */
    int getVolume4Bits() const noexcept;

    /** @return the reference frequency, in hz (440 for example). */
    float getReferenceFrequency() const noexcept;

    /** @return true if high-priority (heard over anything else, cannot be stopped). */
    bool isHighPriority() const noexcept;

    /** @return true to ignore the sample player frequency compensation. */
    bool mustForceSampleFrequencyForReplay() const noexcept;

private:
    bool empty;

    std::shared_ptr<Sample> sample;

    float amplification;
    Loop loop;
    int sampleFrequencyHz;
    float pitchHz;
    float referenceFrequencyHz;
    int volume4bits;

    bool playSampleFromStart;
    bool highPriority;
    bool forceSampleFrequencyForReplay;
};

}   // namespace arkostracker
