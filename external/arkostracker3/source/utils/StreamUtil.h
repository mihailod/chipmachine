#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Utility class for Streams. */
class StreamUtil
{
public:
    /** Prevents instantiation. */
    StreamUtil() = delete;

    /**
     * Extracts a String from the given InputStream, at the given output. Return carriage is not present in the extracted String.
     * The String must not end with a 0.
     * @param inputStream the Input Stream to read. If not enough byte, failure is returned.
     * @param size the size to read.
     * @param successOut filled with true if success, else false.
     * @return the String, or empty if failure.
     */
    static juce::String extractString(juce::InputStream& inputStream, int size, bool& successOut) noexcept;

    /**
     * Finds the given String inside the given InputStream. Useful to check a header at the beginning, for example.
     * @param inputStream the input stream.
     * @param sizeToRead how many bytes to read from the beginning.
     * @param stringToFind the String to find.
     * @param resetStreamAfterSearch if true, the Stream is reset after the search. Useful to make several searches for example or reuse the Stream.
     * @return true if the String was found. False if not, or if the given amount couldn't be read.
     */
    static bool findStringInStream(juce::InputStream& inputStream, int sizeToRead, const juce::String& stringToFind, bool resetStreamAfterSearch = false) noexcept;

    /**
     * @return true if the following String matches the given String.
     * @param inputStream the stream. It will advance.
     * @param string the String to compare to.
     */
    static bool readAndCompareString(juce::InputStream& inputStream, const juce::String& string) noexcept;

    /**
     * Writes a UTF-8 String into the given output stream.
     * @param outputStream the output stream.
     * @param text the text.
     * @param addReturnCarriage true to add a return carriage.
     */
    static void write(juce::OutputStream& outputStream, const juce::String& text, bool addReturnCarriage = true) noexcept;
};

}   // namespace arkostracker
