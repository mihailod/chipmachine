#pragma once

namespace arkostracker 
{

class SamplePart;
class SampleEncoderFlags;
class Song;
class SourceGenerator;

/**
 * Class to easily generate Sample code from a Sample Part.
 * WARNING, this does NOT apply the resampling from the diginote! Use the SampleResampler first if needed.
 */
class SampleCodeGenerator
{
public:
    /** Prevents construction. */
    SampleCodeGenerator() = delete;

    /**
     * Generates a generic header for a Sample Instrument.
     * @param sourceGenerator the Source Generator where to encode the sources.
     * @param sampleEncoderFlags the flags to indicate how to encode the sample.
     * @param samplePart the Sample Part to encode.
     * @return true if everything went fine. False if the sample is too big, or not a Sample Instrument.
     */
    static bool generateHeader(SourceGenerator& sourceGenerator, const SampleEncoderFlags& sampleEncoderFlags, const SamplePart& samplePart) noexcept;

    /**
     * Generates the source for a Sample Instrument.
     * @param sourceGenerator the Source Generator where to encode the sources.
     * @param sampleEncoderFlags the flags to indicate how to encode the sample.
     * @param samplePart the Sample Part to encode.
     */
    static void generateSampleData(SourceGenerator& sourceGenerator, const SampleEncoderFlags& sampleEncoderFlags, const SamplePart& samplePart) noexcept;
};

}   // namespace arkostracker
