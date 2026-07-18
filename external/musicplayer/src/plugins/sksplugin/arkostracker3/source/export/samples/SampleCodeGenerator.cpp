#include "SampleCodeGenerator.h"

#include "../../song/instrument/sample/SamplePart.h"
#include "../../utils/NumberUtil.h"
#include "../sourceGenerator/SourceGenerator.h"
#include "SampleEncoder.h"

namespace arkostracker 
{

bool SampleCodeGenerator::generateHeader(SourceGenerator& sourceGenerator, const SampleEncoderFlags& sampleEncoderFlags, const SamplePart& samplePart) noexcept
{
    jassert(sampleEncoderFlags.areSamplesExported());
    const auto sample = samplePart.getSample();

    // The length is not only the length, but also depends on the possible size alignment.
    const auto originalLength = sample->getLength();
    const auto encodedLength = SampleEncoder::calculateGeneratedSampleLength(originalLength, sampleEncoderFlags.getPaddingLength());
    // Security about the sample length! Else "dw xxxx" will fail.
    if (encodedLength > 65535) {
        return false;
    }

    const auto loop = samplePart.getLoop();
    const auto loopStartIndex = loop.getStartIndex();
    const auto endIndex = loop.getEndIndex();
    const auto isLooping = loop.isLooping();

    // Old header, too verbose.
    /*sourceGenerator.declareWord(encodedLength, "Length (including padding).");
    sourceGenerator.declareWord(endIndex, "End index.");
    sourceGenerator.declareWord(loopStartIndex, "Loop start index.");
    sourceGenerator.declareByte(NumberUtil::boolToInt(isLooping), "Loop?");

    sourceGenerator.declareByte(sampleEncoderFlags.getAmplitude() - 1, "Amplitude (8 bits) - 1.");
    sourceGenerator.declareByte(sampleEncoderFlags.getOffset(), "Offset added to the each value.");*/

    // New header.
    sourceGenerator.declarePointerRegionStart();
    sourceGenerator.declareWord("$ + 6", "Sample address.");
    sourceGenerator.declareWord("$ + 4 + " + juce::String(endIndex), "End address.");
    if (isLooping) {
        sourceGenerator.declareWord("$ + 2 + " + juce::String(loopStartIndex), "Loop address.");
    } else {
        sourceGenerator.declareWord(0, "No loop.");
    }
    sourceGenerator.declarePointerRegionEnd();

    sourceGenerator.addEmptyLine();

    return true;
}

void SampleCodeGenerator::generateSampleData(SourceGenerator& sourceGenerator, const SampleEncoderFlags& sampleEncoderFlags, const SamplePart& samplePart) noexcept
{
    jassert(sampleEncoderFlags.areSamplesExported());

    sourceGenerator.declareComment("The sample data.");

    const auto sample = samplePart.getSample();

    // Converts the sample according to what the user wants.
    const SampleEncoder sampleEncoder(sampleEncoderFlags);
    const auto loop = samplePart.getLoop();
    const auto loopStartIndex = loop.getStartIndex();
    const auto endIndex = loop.getEndIndex();
    const auto isLooping = loop.isLooping();
    const auto amplificationRatio = samplePart.getAmplificationRatio();

    const auto samplesToEncode = sampleEncoder.convert(sample->getData(), amplificationRatio, isLooping, loopStartIndex, endIndex);

    // Encodes all the bytes!
    sourceGenerator.declareByteRegionStart();
    for (const auto sampleToEncode : *samplesToEncode) {
        sourceGenerator.declareByte(sampleToEncode);
    }
    sourceGenerator.declareByteRegionEnd();
    sourceGenerator.addEmptyLine();
}

}   // namespace arkostracker
