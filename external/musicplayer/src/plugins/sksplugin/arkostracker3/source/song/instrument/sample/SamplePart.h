#pragma once

#include "../../../business/model/Loop.h"
#include "../../../utils/PsgValues.h"
#include "../../psg/PsgFrequency.h"
#include "Sample.h"

namespace arkostracker 
{

/** This is the Sample part of an Instrument. */
class SamplePart
{
public:
    static constexpr auto defaultAmplificationRatio = 1.0F;

    /** Constructor with no sample. */
    SamplePart() noexcept;

    /**
     * Constructor.
     * @param sample the sample. It MUST be then put in the SampleManager via the fill method.
     * @param loop the loop.
     * @param amplificationRatio the amplification ratio (1.0 by default).
     * @param frequencyHz the frequency of the sample, in hz.
     * @param digidrumNote the note used for digidrums (events).
     * @param originalFileName the original loaded file name ("SuperDrum.wav" for example). May be blank.
     */
    SamplePart(std::shared_ptr<Sample> sample, Loop loop, float amplificationRatio = defaultAmplificationRatio,
        int frequencyHz = PsgFrequency::defaultSampleFrequencyHz, int digidrumNote = PsgValues::digidrumNote,
        juce::String originalFileName = juce::String()) noexcept;

    /**
     * Sets the sample. It MUST be then put in the SampleManager via the addSample method.
     * @param sample the sample.
     * @param loop the loop.
     * @param amplificationRatio the amplification ratio (1.0 by default).
     */
    void setSample(std::shared_ptr<Sample> sample, const Loop& loop, float amplificationRatio = 1.0F) noexcept;

    /** @return the sample. */
    std::shared_ptr<Sample> getSample() const noexcept;
    /** @return the frequency, in Hz. */
    int getFrequencyHz() const noexcept;
    /** @return the loop. */
    Loop getLoop() const noexcept;
    /** @return the amplification ratio. */
    float getAmplificationRatio() const noexcept;

    /** The original loaded file name ("SuperDrum.wav" for example). May be blank. */
    juce::String getOriginalFileName() const noexcept;

    /** Sets the loop. */
    void setLoop(const Loop& loop) noexcept;
    /** Sets the amplification ratio (1.0 for example). */
    void setAmplificationRatio(float amplificationRatio) noexcept;
    /** Sets the frequency, in hz. */
    void setFrequencyHz(int frequencyHz) noexcept;
    /** Sets the original file name ("drums.wav" for example). */
    void setOriginalFilename(juce::String fileName) noexcept;

    /** @return the note used for digidrums (events). */
    int getDigidrumNote() const noexcept;
    /**
     * Sets the digidrum notes (events).
     * @param digidrumNote the note.
     */
    void setDigidrumNote(int digidrumNote) noexcept;

    bool operator==(const SamplePart& other) const noexcept;
    bool operator!=(const SamplePart& other) const noexcept;

private:
    std::shared_ptr<Sample> sample;

    int frequencyHz;
    Loop loop;
    float amplificationRatio;

    int digidrumNote;                    // Note used for digidrums (events).

    juce::String originalFileName;
};

}   // namespace arkostracker
