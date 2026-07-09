#include "SampleEncoder.h"

#include <algorithm>

#include "../../audio/effect/AudioUtil.h"
#include "../../utils/NumberUtil.h"

namespace arkostracker 
{

SampleEncoder::SampleEncoder(const SampleEncoderFlags& pSampleEncoderFlags) noexcept :
    sampleEncoderFlags(pSampleEncoderFlags)
{
}

std::unique_ptr<std::vector<unsigned char>> SampleEncoder::convert(const juce::MemoryBlock& sampleData, const float amplification,
                                                                   const bool loop, const int loopToIndex, const int endIndex) const noexcept
{
    auto output = std::make_unique<std::vector<unsigned char>>();

    const auto sampleSize = sampleData.getSize();
    jassert(endIndex < static_cast<int>(sampleSize));     // Else, crash ahead!

    // Converts each sample.
    size_t i = 0;

    const auto exportOnlyUsedLength = sampleEncoderFlags.isOnlyExportUsedLength();
    const auto paddingLength = sampleEncoderFlags.getPaddingLength();

    // The length may be the used length, if wanted.
    // However, if the sound is looping and that there is padding, it is logical that only the data up to the end index is used, because
    // the padding must loop on the... looping data only.
    const auto maximumIndex = (exportOnlyUsedLength || (loop && (paddingLength > 0))) ? static_cast<size_t>(endIndex) : (sampleSize - 1);

    //for (unsigned char sample : samples) {
    for (size_t index = 0; index < sampleSize; ++index) {
        const auto sample = sampleData[index];
        output->push_back(processSample(static_cast<unsigned char>(sample), amplification));

        if (++i > maximumIndex) {
            break;
        }
    }

    // Manages the possible fade out.
    manageFadeOut(maximumIndex, loop, endIndex, *output);

    // Manages the possible padding.
    managePadding(loop, loopToIndex, endIndex, amplification, *output);

    // Adds the offset. We don't care about the overflow.
    const auto offset = static_cast<unsigned char>(sampleEncoderFlags.getOffset());
    for (auto& sample : *output) {
        sample += offset;
    }

    return output;
}

void SampleEncoder::manageFadeOut(const size_t maximumIndex, const bool loop, const int endIndex, std::vector<unsigned char>& output) const noexcept
{
    // Should the latest samples be faded out? Only for non-looping samples.
    const auto originalFadeOutLength = sampleEncoderFlags.getFadeOutSampleCountForNonLoopingSamples();
    if (loop || (originalFadeOutLength == 0)) {
        return;
    }

    // Makes sure the fade out length is not too large. Restricts it to the end index.
    const auto fadeOutLength = static_cast<unsigned int>((originalFadeOutLength >= endIndex) ? endIndex : originalFadeOutLength);
    jassert(fadeOutLength < output.size());

    // Must fade Targets 0 or 0.5?
    const auto mustCompensateVertically = !sampleEncoderFlags.isForcePaddingAndFadeOutToLowestValue();
    const auto step = -1.0 / static_cast<double>(fadeOutLength);
    auto currentFadeOutRatio = 1.0 + step;        // Starts the fade directly.

    const auto middleValue = sampleEncoderFlags.getAmplitude();
    for (auto fadeOutIndex = maximumIndex - fadeOutLength + 1; fadeOutIndex <= maximumIndex; ++fadeOutIndex) {
        auto value = static_cast<double>(output.at(fadeOutIndex)) * currentFadeOutRatio;

        // Compensates vertically? Yes to "center" the signal.
        if (mustCompensateVertically) {
            value += static_cast<double>(middleValue) / 2.0 * (1.0 - currentFadeOutRatio);
        }
        jassert((value >= 0) && (value < 256));

        output[fadeOutIndex] = static_cast<unsigned char>(value);

        currentFadeOutRatio += step;
        currentFadeOutRatio = std::max(currentFadeOutRatio, 0.0);             // May slightly overflow, security.
    }
}

void SampleEncoder::managePadding(const bool loop, const int loopIndex, const int endIndex, const float amplification, std::vector<unsigned char>& output) const noexcept
{
    // Padding to add? Only if not looping!
    const auto paddingLength = static_cast<size_t>(sampleEncoderFlags.getPaddingLength());
    if (paddingLength == 0) {
        return;
    }

    if (loop) {
        // Looping. Duplicates the bytes from the loop Start to loop End, using a modulo to manage the (unlikely) case of a verrry long padding length.
        const auto loopLength = endIndex - loopIndex + 1;
        for (size_t i = 0; i < paddingLength; ++i) {
            const auto position = (i % static_cast<size_t>(loopLength)) + static_cast<size_t>(loopIndex);

            auto paddingByte = output.at(position);
            output.push_back(paddingByte);
        }
    } else {
        // Not looping. Adds the padding bytes.
        const auto originalSize = output.size();
        const auto finalSize = calculateGeneratedSampleLength(static_cast<int>(originalSize), static_cast<int>(paddingLength));

        // The padding byte may be the lowest, or the "middle".
        const auto basePaddingByte = static_cast<unsigned char>(sampleEncoderFlags.isForcePaddingAndFadeOutToLowestValue() ? 0 : 128);
        const auto paddingByte = processSample(basePaddingByte, amplification);

        output.resize(static_cast<size_t>(finalSize), paddingByte);
    }
}

unsigned char SampleEncoder::processSample(const unsigned char inputSample, const float amplification) const noexcept
{
    auto amplitude = sampleEncoderFlags.getAmplitude();
    jassert((amplitude > 0) && (amplitude <= 256));
    amplitude = NumberUtil::correctNumber(amplitude, 1, 256);

    // Applies the amplification.
    const auto amplifiedSample = amplifySampleValue(inputSample, amplification);

    // Applies the amplitude. This becomes the maximum value, excluded.
    auto sampleFloat = static_cast<float>(amplifiedSample);
    jassert((amplitude > 0) && (amplitude <= 256));
    if ((amplitude > 0) && (amplitude != 256)) {        // If 256, nothing to do!
        sampleFloat = sampleFloat * static_cast<float>(amplitude) / 256.0F;
    }

    // Checks its limit, from within the amplitude - 1, because 256 (for example) must never be reached.
    return NumberUtil::correctNumber(static_cast<unsigned char>(sampleFloat), static_cast<unsigned char>(0), static_cast<unsigned char>(amplitude - 1));
}

int SampleEncoder::calculateGeneratedSampleLength(const int originalLength, const int paddingLength) noexcept
{
    return originalLength + paddingLength;
}

}   // namespace arkostracker
