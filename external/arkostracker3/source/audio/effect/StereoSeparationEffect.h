#pragma once

#include "BaseEffect.h"

namespace arkostracker
{

/**
 * Effect that manages stereo separation (this includes mono management).
 */
class StereoSeparationEffect : public BaseEffect
{
public:
    StereoSeparationEffect() noexcept;

    /**
     * Sets the separation value.
     * @param separation the separation, from 1.0 (full) to 0.0 (mono).
     */
    void setSeparation(double separation) noexcept;

    // BaseEffect method implementations.
    // ==========================================================
    void prepareToPlay(double sampleRate) noexcept override;
    void releaseResources() override;

protected:
    // BaseEffect method implementations.
    // ==========================================================
    void processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, int afterLastPositionInBuffer) noexcept override;

private:
    /** The stereo separation, from 0 (mono) to 1 (full). */
    double separation;
};

}   // namespace arkostracker
