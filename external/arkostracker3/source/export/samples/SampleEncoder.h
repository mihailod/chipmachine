#pragma once

#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "SampleEncoderFlags.h"

namespace arkostracker 
{

/**
 * Encodes a sample according to the given flags (signed, bit rate, etc.).
 */
class SampleEncoder
{
public:

    /**
     * Constructor.
     * @param sampleEncoderFlags the flags to know how to export.
     */
    explicit SampleEncoder(const SampleEncoderFlags& sampleEncoderFlags) noexcept;

    /**
     * Converts the given sample.
     * @param sample the sample to convert.
     * @param amplification a ratio to which each sample must be multiplied to.
     * @param loop true if the sample is looping.
     * @param loopToIndex the loop to index. May be 0 if not looping.
     * @param endIndex the end index of the sample. May be inferior to the (length - 1) of course, but must be inferior to it.
     * @return the converted sample.
    */
    std::unique_ptr<std::vector<unsigned char>> convert(const juce::MemoryBlock& sample, float amplification, bool loop,
                                                        int loopToIndex, int endIndex) const noexcept;

    /**
     * Calculates the length of the generated sample. This varies according to the given padding.
     * @param originalLength the original length.
     * @param paddingLength a possible padding (>=0).
     * @return the length.
     */
    static int calculateGeneratedSampleLength(int originalLength, int paddingLength) noexcept;

private:
    /**
     * Manages the fade-out, if any.
     * @param maximumIndex the maximum index inside the sample. May be the end index, or the last byte.
     * @param loop true if the sample is looping.
     * @param endIndex the end index of the sample. May be inferior to the (length - 1) of course.
     * @param output the output data to modify the data.
     */
    void manageFadeOut(size_t maximumIndex, bool loop, int endIndex, std::vector<unsigned char>& output) const noexcept;

    /**
     * Manages the padding.
     * @param loop true if the sample is looping.
     * @param loopIndex the index where the sample loops.
     * @param endIndex the end index, for the loop.
     * @param amplification a ratio to which each sample must be multiplied to.
     * @param output the output data to modify the data.
     */
    void managePadding(bool loop, int loopIndex, int endIndex, float amplification, std::vector<unsigned char>& output) const noexcept;

    /**
        Processes one sample, producing a sample to encode.
        @param inputSample the sample to process.
        @param amplification a ratio to which each sample must be multiplied to.
        @return the processedSample.
    */
    unsigned char processSample(unsigned char inputSample, float amplification) const noexcept;

    SampleEncoderFlags sampleEncoderFlags;                            // Flags to know how to export.
};

}   // namespace arkostracker
