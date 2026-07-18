#pragma once

#include "BaseEffect.h"

namespace arkostracker
{

/** Simple global volume effect. */
class VolumeEffect : public BaseEffect
{
public:
    VolumeEffect() noexcept;

    /**
     * Sets the separation value.
     * @param volume the volume, from 1.0 (full) to 0.0 (mute).
     */
    void setVolume(double volume) noexcept;

    // BaseEffect method implementations.
    // ==========================================================
    void prepareToPlay(double sampleRate) noexcept override;
    void releaseResources() override;

protected:
    // BaseEffect method implementations.
    // ==========================================================
    void processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, int afterLastPositionInBuffer) noexcept override;

private:
    /** The volume, from 0 (mute) to 1 (full). */
    double volume;
};

}   // namespace arkostracker
