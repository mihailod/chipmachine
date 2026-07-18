#include "SamplePart.h"

#include <utility>

namespace arkostracker
{

SamplePart::SamplePart() noexcept :
        sample(),
        frequencyHz(PsgFrequency::defaultSampleFrequencyHz),
        loop(),
        amplificationRatio(defaultAmplificationRatio),
        digidrumNote(PsgValues::digidrumNote),
        originalFileName()
{
}

SamplePart::SamplePart(std::shared_ptr<Sample> pSample, const Loop pLoop, const float pAmplificationRatio, const int pFrequencyHz,
    const int pDigidrumNote, juce::String pOriginalFileName) noexcept :
        sample(std::move(pSample)),
        frequencyHz(pFrequencyHz),
        loop(pLoop),
        amplificationRatio(pAmplificationRatio),
        digidrumNote(pDigidrumNote),
        originalFileName(std::move(pOriginalFileName))
{
}

void SamplePart::setSample(std::shared_ptr<Sample> newSample, const Loop& newLoop, const float newAmplificationRatio) noexcept
{
    sample = std::move(newSample);
    loop = newLoop;
    amplificationRatio = newAmplificationRatio;
}

std::shared_ptr<Sample> SamplePart::getSample() const noexcept
{
    return sample;
}

int SamplePart::getFrequencyHz() const noexcept
{
    return frequencyHz;
}

Loop SamplePart::getLoop() const noexcept
{
    return loop;
}

float SamplePart::getAmplificationRatio() const noexcept
{
    return amplificationRatio;
}

juce::String SamplePart::getOriginalFileName() const noexcept
{
    return originalFileName;
}

void SamplePart::setLoop(const Loop& newLoop) noexcept
{
    loop = newLoop;
}

void SamplePart::setAmplificationRatio(const float newAmplificationRatio) noexcept
{
    amplificationRatio = newAmplificationRatio;
}

void SamplePart::setFrequencyHz(const int newFrequencyHz) noexcept
{
    frequencyHz = newFrequencyHz;
}

void SamplePart::setOriginalFilename(juce::String fileName) noexcept
{
    originalFileName = std::move(fileName);
}

int SamplePart::getDigidrumNote() const noexcept
{
    return digidrumNote;
}

void SamplePart::setDigidrumNote(const int pDigidrumNote) noexcept
{
    digidrumNote = pDigidrumNote;
}

bool SamplePart::operator==(const SamplePart& other) const noexcept
{
    if (const auto equal = (frequencyHz == other.frequencyHz)
                           && (loop == other.loop)
                           && (digidrumNote == other.digidrumNote)
                           && juce::exactlyEqual(amplificationRatio, other.amplificationRatio); !equal
                           && originalFileName == other.originalFileName) {
        return false;
    }

    if (sample == nullptr) {
        return (other.sample == nullptr);
    }

    // The sample is present.
    if (other.sample == nullptr) {
        return false;
    }

    // Both are present.
    return (*sample == *other.sample);
}

bool SamplePart::operator!=(const SamplePart& other) const noexcept
{
    return !(other == *this);
}

}   // namespace arkostracker
