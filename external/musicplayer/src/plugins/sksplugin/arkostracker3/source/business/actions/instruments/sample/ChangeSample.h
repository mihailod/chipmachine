#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../../song/instrument/sample/Sample.h"
#include "../../../../utils/Id.h"
#include "../../../model/Loop.h"

namespace arkostracker
{

class SongController;

/** Changes a sample in a Sample Instrument. */
class ChangeSample final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the instrument ID. It must exist and be a Sample instrument.
     * @param sample the new sample.
     * @param sampleFrequencyHz the sample frequency, in Hz.
     * @param originalFilename the name of the original file name ("Drum.wav" for example).
     */
    ChangeSample(SongController& songController, Id id, std::unique_ptr<Sample> sample, int sampleFrequencyHz, juce::String originalFilename) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the listeners of a change. */
    void notify() const noexcept;

    SongController& songController;
    const Id id;

    std::shared_ptr<Sample> sample;
    int sampleFrequencyHz;
    juce::String originalFilename;

    Loop oldLoop;
    float oldAmplificationRatio;
    std::shared_ptr<Sample> oldSample;
    int oldSampleFrequencyHz;
    juce::String oldOriginalFilename;
};

}   // namespace arkostracker
