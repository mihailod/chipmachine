#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Class that create a Sample from a File. */
class SampleLoader
{
public:
    enum class Outcome : uint8_t
    {
        success,
        fileNotFound,
        moreThan128k,
        formatUnknown,
        parsingError,
    };

    class Result
    {
    public:
        Result(std::unique_ptr<juce::MemoryBlock> pData, const int pSampleFrequencyHz, juce::String pOriginalFileName, const Outcome pOutcome) :
                data(std::move(pData)),
                sampleFrequencyHz(pSampleFrequencyHz),
                originalFileName(std::move(pOriginalFileName)),
                outcome(pOutcome)
        {
        }

        std::unique_ptr<juce::MemoryBlock> getData() noexcept
        {
            return std::move(data);
        }

        juce::String getOriginalFileName() const noexcept
        {
            return originalFileName;
        }

        Outcome getOutcome() const noexcept
        {
            return outcome;
        }

        int getSampleFrequencyHz() const noexcept
        {
            return sampleFrequencyHz;
        }

    private:
        std::unique_ptr<juce::MemoryBlock> data;
        int sampleFrequencyHz;
        juce::String originalFileName;
        Outcome outcome;
    };

    /** Prevents instantiation. */
    SampleLoader() = delete;

    /**
     * @return object containing a memory block of the sample data (unsigned, from 0 to 255), or nullptr if an error occurred.
     * @param fileToLoad the file to load. May not exist.
     * @param canLoadMoreThan128k true if loading a more than 128k file is allowed.
     */
    static std::unique_ptr<Result> readFile(const juce::File& fileToLoad, bool canLoadMoreThan128k = false) noexcept;

    /**
     * @return object containing a memory block of the sample data (unsigned, from 0 to 255), or nullptr if an error occurred.
     * @param reader the reader (from a file, a stream).
     * @param originalFileName the possible original file name, to put back into the result.
     */
    static std::unique_ptr<Result> read(juce::AudioFormatReader& reader, const juce::String& originalFileName) noexcept;

    /** @return a signed char from the given float. */
    static signed char floatToSignedChar(float value) noexcept;
};

}   // namespace arkostracker
