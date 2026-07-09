#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace arkostracker
{

/** Abstract class to process a given audio buffer. */
class BaseEffect
{
public:
    BaseEffect();

    /** Destructor. */
    virtual ~BaseEffect() = default;

    /** Sets the effect to enabled or not.  Can be called from any thread. */
    void setEnabled(bool enabled) noexcept;

    /**
     * Called from the audio thread when the sample rate is known. The implementation can start making some pre-calculations.
     * @param sampleRate the sample rate (44100, for example).
     */
    virtual void prepareToPlay(double sampleRate) noexcept = 0;

    /**
     * Processes the given buffer. This will probably be called on the audio thread, so don't waste time!
     * @param audioSourceChannelInfo data about the audio source channels.
     */
    void processBuffer(const juce::AudioSourceChannelInfo& audioSourceChannelInfo) noexcept;

    /** Asks the effect to release its resources. Will be called once before prepareToPlay is called again. */
    virtual void releaseResources() = 0;

protected:
    /**
     * Implements this to process the given buffer, within the given range.
     * @param buffer the buffer.
     * @param positionInBuffer the first position in buffer to process.
     * @param afterLastPositionInBuffer the index after the last position to process in the buffer.
     */
    virtual void processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, int afterLastPositionInBuffer) noexcept = 0;

private:
    std::atomic_bool enabled;
};

}   // namespace arkostracker
