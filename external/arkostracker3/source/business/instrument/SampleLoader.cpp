#include "SampleLoader.h"

#include "../../song/instrument/sample/Sample.h"
#include "../../utils/NumberUtil.h"

namespace arkostracker
{

std::unique_ptr<SampleLoader::Result> SampleLoader::readFile(const juce::File& fileToLoad, const bool canLoadMoreThan128k) noexcept
{
    const auto originalFileName = fileToLoad.getFileName();

    if (!fileToLoad.existsAsFile()) {
        return std::make_unique<Result>(nullptr, 0, originalFileName, Outcome::fileNotFound);
    }

    // Attempts to read the file.
    juce::AudioFormatManager audioFormatManager;
    audioFormatManager.registerBasicFormats();
    const std::unique_ptr<juce::AudioFormatReader> reader(audioFormatManager.createReaderFor(fileToLoad));
    if (reader == nullptr) {
        return std::make_unique<Result>(nullptr, 0, originalFileName, Outcome::formatUnknown);
    }

    // Not empty?
    const auto length = static_cast<int>(reader->lengthInSamples);
    if (length <= 0) {
        return std::make_unique<Result>(nullptr, 0, originalFileName, Outcome::parsingError);
    }
    // Not too large?
    if (!canLoadMoreThan128k && (length > Sample::maximumLength)) {
        return std::make_unique<Result>(nullptr, 0, originalFileName, Outcome::moreThan128k);
    }

    return read(*reader, originalFileName);
}

std::unique_ptr<SampleLoader::Result> SampleLoader::read(juce::AudioFormatReader& reader, const juce::String& originalFileName) noexcept
{
    const auto length = static_cast<int>(reader.lengthInSamples);

    // Converts to float.
    juce::AudioBuffer<float> audioBuffer;
    audioBuffer.setSize(static_cast<int>(reader.numChannels), length);
    const auto success = reader.read(&audioBuffer, 0, length,
        0, true, false);
    if (!success) {
        return std::make_unique<Result>(nullptr, 0, originalFileName, Outcome::parsingError);
    }

    const auto sampleFrequencyHz = static_cast<int>(reader.sampleRate);

    // Converts to 0-255.
    auto sampleMemoryBlock = std::make_unique<juce::MemoryBlock>();
    sampleMemoryBlock->ensureSize(static_cast<size_t>(length));
    for (auto i = 0; i < length; ++i) {
        const auto sampleFloat = audioBuffer.getSample(0, i);
        const auto sampleChar = floatToSignedChar(sampleFloat);
        sampleMemoryBlock->operator[](i) = static_cast<char>(sampleChar);  // Re(!)-forced to char for ARM, else error.
    }

    return std::make_unique<Result>(std::move(sampleMemoryBlock), sampleFrequencyHz, originalFileName, Outcome::success);
}

signed char SampleLoader::floatToSignedChar(const float value) noexcept
{
    const auto sample128 = static_cast<int>(value * 128.0);
    return static_cast<signed char>(NumberUtil::correctNumber(sample128, -128, 127) + 128);
}

}   // namespace arkostracker
