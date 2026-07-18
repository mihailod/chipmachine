#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Helper for ZIP/GZIP/LZH/LHA streams. */
class ZipHelper
{
public:
    /** Prevents instantiation. */
    ZipHelper() = delete;

    /**
     * Tries to unzip the given stream, trying with ZIP, GZIP, then LZH/LHZ. The first must only contain one file, else it is considered not a ZIP file.
     * @param inputStream the input stream. As a security, will be reset first.
     * @return an unzipped stream, or nullptr if it couldn't be unzipped.
     */
    static std::unique_ptr<juce::InputStream> unzip(juce::InputStream& inputStream) noexcept;

    /**
     * @return a new memory block, which may be the same as the given one.
     * @param inputMemoryBlock the input memory block, zipped (ZIP, GZIP, then LZH/LHZ) or not.
     */
    static juce::MemoryBlock unzip(const juce::MemoryBlock& inputMemoryBlock) noexcept;

private:
    /**
     * @return an unpacked GZIP stream, or nullptr if it couldn't.
     * @param inputStream the input stream.
     */
    static std::unique_ptr<juce::InputStream> depackGzip(juce::InputStream& inputStream) noexcept;

    /**
     * @return an unpacked LHA/LZH stream, or nullptr if it couldn't.
     * @param inputStream the input stream.
     */
    static std::unique_ptr<juce::InputStream> depackLha(juce::InputStream& inputStream) noexcept;
};

}   // namespace arkostracker
