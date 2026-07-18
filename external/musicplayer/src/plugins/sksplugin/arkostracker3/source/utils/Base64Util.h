#pragma once

#include <juce_core/juce_core.h>

#include <array>

namespace arkostracker 
{

/**
 * Utility class to encode/decode Base64.
 * This is mostly useful to encode/decode binary data (such as samples) to XML.
 */
class Base64Util
{
public:
    /** Deleted constructor. */
    Base64Util() = delete;

    /**
     * Encodes in Base64 the given stream to the given OutputStream.
     * @param inputStream the stream to encode.
     * @param outputStream the stream to fill with Base64 data.
     * @return true if everything went fine.
     */
    static bool encodeToBase64(juce::InputStream& inputStream, juce::OutputStream& outputStream) noexcept;

    /**
     * Encodes in Base64. This is slow and should be used for small Strings.
     * @param inputText the text to encode.
     * @param success true if everything went fine.
     * @return the base-64 String, or empty if a problem occurred.
     */
    static juce::String encodeToBase64(const juce::String &inputText, bool& success) noexcept;      // TODO TU.

    /**
     * Decodes the Base64 given stream to the given OutputStream.
     * @param inputStream the Base64 stream to read from.
     * @param outputStream the stream where to fill with decoded data.
     * @return true if everything went fine.
     */
    static bool decodeFromBase64(juce::InputStream& inputStream, juce::OutputStream& outputStream) noexcept;

    /**
     * Utility method to convert a Base64 String to a vector of unsigned char. This is not very efficient and should be
     * used for small Strings only.
     * @param inputText the Base64 input text.
     * @param success a value used as a return type to indicate whether the conversion was successful or not.
     */
    static std::vector<unsigned char> decodeBase64StringToVector(const juce::String &inputText, bool &success) noexcept;

private:
    static constexpr unsigned char paddingChar = '=';           // Character used for padding.
    static const unsigned char paddingIndex;                    // Index of a padding character.
    static const std::array<unsigned char, 64> base64Index;     // Look-up table converting an index to an Ascii character.
    static const std::array<unsigned char, 256> charToIndex;    // Look-up table converting an Ascii character to an index, or 255 if invalid.
};

}   // namespace arkostracker
