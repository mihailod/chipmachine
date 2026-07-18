#include "StreamUtil.h"

namespace arkostracker 
{

juce::String StreamUtil::extractString(juce::InputStream& inputStream, const int size, bool& successOut) noexcept
{
    // Enough data left?
    if (inputStream.getNumBytesRemaining() < size) {
        successOut = false;
        return { };
    }

    successOut = true;

    // Not the most glorious code...
    juce::MemoryBlock buffer(static_cast<size_t>(size), false);

    inputStream.read(buffer.getData(), size);
    const juce::CharPointer_UTF8 pointer(static_cast<const juce::CharPointer_UTF8::CharType*>(buffer.getData()));
    juce::String readString(pointer, static_cast<size_t>(size));

    return readString;
}

bool StreamUtil::findStringInStream(juce::InputStream& inputStream, const int sizeToRead, const juce::String& stringToFind, const bool resetStreamAfterSearch) noexcept
{
    juce::MemoryBlock memoryBlock;
    // If the bytes couldn't be read, it's surely because the file is not right!
    if (const auto readBytes = inputStream.readIntoMemoryBlock(memoryBlock, sizeToRead); readBytes != static_cast<size_t>(sizeToRead)) {
        return false;
    }

    // If wanted, resets the Stream.
    if (resetStreamAfterSearch) {
        const auto reset = inputStream.setPosition(0);
        jassert(reset); (void)reset;            // To avoid a warning in Release.
    }

    const auto data = memoryBlock.toString();           // No need to have a 0-terminated String, it seems.

    // Is our tag here?
    return (data.contains(stringToFind));
}

bool StreamUtil::readAndCompareString(juce::InputStream& inputStream, const juce::String& string) noexcept
{
    auto success = false;
    const auto readString = extractString(inputStream, string.length(), success);

    return success && (readString == string);
}

void StreamUtil::write(juce::OutputStream& outputStream, const juce::String& text, const bool addReturnCarriage) noexcept
{
    outputStream.writeText(text, false, false, nullptr);

    if (addReturnCarriage) {
        outputStream.writeText(outputStream.getNewLineString(), false, false, nullptr);
    }
}

}   // namespace arkostracker
