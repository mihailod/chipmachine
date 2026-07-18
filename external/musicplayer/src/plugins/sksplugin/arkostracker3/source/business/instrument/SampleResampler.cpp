#include "SampleResampler.h"

#include "../../player/TemperedScaleUtil.h"
#include "../../utils/NumberUtil.h"

namespace arkostracker
{

SamplePart SampleResampler::resample(const SamplePart& inputSamplePart, const double pitchCents) noexcept
{
    // If the same diginote and pitch, returns the same data (though copied...).
    const auto newDigidrumNote = inputSamplePart.getDigidrumNote();
    if ((newDigidrumNote == PsgValues::digidrumNote) && (juce::exactlyEqual(pitchCents, 0.0))) {
        return inputSamplePart;
    }

    const auto& inputSample = inputSamplePart.getSample();
    const auto inputSampleSize = inputSample->getLength();

    // Performs the resampling.
    const auto& [newSampleData, sampleStep] = resample(inputSample->getData(), inputSampleSize, newDigidrumNote, pitchCents);

    const auto inputLoop = inputSamplePart.getLoop();
    const auto newSample = std::make_shared<Sample>(newSampleData);
    const auto newSampleLastIndex = static_cast<int>(newSampleData.getSize()) - 1;
    // For the end, makes sure we go past the original index to reach the "final" sample.
    const auto newEndIndexDouble = std::ceil((inputLoop.getEndIndex() + 1) / sampleStep);
    // Added a security to avoid going over the bounds.
    const auto newEndIndex = NumberUtil::correctNumber(static_cast<int>(newEndIndexDouble) - 1, 0, newSampleLastIndex);
    const auto newStartIndex = NumberUtil::correctNumber(static_cast<int>(inputLoop.getStartIndex() / sampleStep), 0, newEndIndex);
    const auto newLoop = Loop(newStartIndex, newEndIndex, inputLoop.isLooping());

    return { newSample, newLoop, inputSamplePart.getAmplificationRatio(), inputSamplePart.getFrequencyHz(),
        PsgValues::digidrumNote, inputSamplePart.getOriginalFileName() };
}

std::pair<juce::MemoryBlock, double> SampleResampler::resample(const juce::MemoryBlock& inputSample, const int inputSampleSize, const int note, const double pitchCents) noexcept
{
    constexpr auto baseNote = PsgValues::digidrumNote;

    const auto isPitchNegative = pitchCents < 0.0;

    const auto pitchFloorNote = static_cast<int>(pitchCents);
    const auto pitchNextNote = pitchFloorNote + (isPitchNegative ? -1 : 1);
    const auto baseDigidrumFrequency = TemperedScaleUtil::getFrequency(baseNote, referenceFrequency);
    const auto floorFrequency = TemperedScaleUtil::getFrequency(note + pitchFloorNote, referenceFrequency);
    const auto nextFrequency = TemperedScaleUtil::getFrequency(note + pitchNextNote, referenceFrequency);

    const auto differenceFrequency = std::abs(nextFrequency - floorFrequency);

    const auto cents = pitchCents - static_cast<int>(pitchCents);
    const auto decimalPitch = differenceFrequency * cents;

    const auto sampleStep = (floorFrequency + decimalPitch) / static_cast<double>(baseDigidrumFrequency);

    return resampleWithStep(inputSample, inputSampleSize, sampleStep);
}

std::pair<juce::MemoryBlock, double> SampleResampler::resampleWithStep(const juce::MemoryBlock& inputSample, const int inputSampleSize, const double step) noexcept
{
    // Sanity check.
    if (step <= 0.0) {
        jassertfalse;       // Invalid step!
        return { inputSample, 1.0 };
    }

    juce::MemoryBlock newSampleData;

    // Performs the resampling.
    char appendedData[1];
    auto index = 0.0;
    while (index < static_cast<double>(inputSampleSize)) {
        const auto readSample = inputSample[static_cast<int>(index)];
        appendedData[0] = readSample;
        newSampleData.append(appendedData, 1);
        index += step;
    }

    return { newSampleData, step };
}

}   // namespace arkostracker
