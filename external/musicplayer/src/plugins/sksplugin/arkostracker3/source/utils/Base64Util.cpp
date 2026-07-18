#include "Base64Util.h"
#include "MemoryBlockUtil.h"

namespace arkostracker 
{

const std::array<unsigned char, 64U> Base64Util::base64Index = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',             // NOLINT(clion-misra-cpp2008-5-0-4)
    'w', 'x', 'y', 'z', '0', '1', '2', '3',             // NOLINT(clion-misra-cpp2008-5-0-4)
    '4', '5', '6', '7', '8', '9', '+', '/'             // NOLINT(clion-misra-cpp2008-5-0-4)
};

const unsigned char Base64Util::paddingIndex = 64U;

const std::array<unsigned char, 256U> Base64Util::charToIndex = {
    // 255 if invalid.

    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,        // 0
    255U, 255U,255U,255U,255U,255U,255U,255U,255U,255U,       // 10
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U, 62U,255U,255U,255U, 63U, 52U, 53U,
     54U, 55U, 56U, 57U, 58U, 59U, 60U, 61U,255U,255U,
    255U,paddingIndex,255U,255U,255U,  0U,  1U,  2U,  3U,  4U,       // 60   61 = '='
      5U,  6U,  7U,  8U,  9U, 10U, 11U, 12U, 13U, 14U,
     15U, 16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U,
     25U,255U,255U,255U,255U,255U,255U, 26U, 27U, 28U,
     29U, 30U, 31U, 32U, 33U, 34U, 35U, 36U, 37U, 38U,
     39U, 40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U, 48U,
     49U, 50U, 51U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U,255U,255U,255U,255U,
    255U,255U,255U,255U,255U,255U
};

bool Base64Util::encodeToBase64(juce::InputStream& inputStream, juce::OutputStream& outputStream) noexcept
{
    jassert(base64Index.size() == 64U);

    auto success = true;

    unsigned int byteToWrite1;    // NOLINT(*-init-variables)
    unsigned int byteToWrite2;    // NOLINT(*-init-variables)
    unsigned int byteToWrite3;    // NOLINT(*-init-variables)
    unsigned int byteToWrite4;    // NOLINT(*-init-variables)
    unsigned int readByte1;       // NOLINT(*-init-variables)
    unsigned int readByte2;       // NOLINT(*-init-variables)
    unsigned int readByte3;       // NOLINT(*-init-variables)
    unsigned int paddingCount;    // NOLINT(*-init-variables)

    auto mustContinue = true;
    while (mustContinue) {
        // Tries to read three chars, though less can be read.
        // Note that we use the fact that reading an exhausting stream returns 0!
        const auto remainingBytes = inputStream.getNumBytesRemaining();
        if (remainingBytes < 0) {
            success = false;
            mustContinue = false;
        } else if (remainingBytes == 0) {
            mustContinue = false;
        } else {
            // There is at least byte to read.
            readByte1 = static_cast<unsigned char>(inputStream.readByte());
            byteToWrite1 = ((readByte1 >> 2U) & 0x3fU);

            if (remainingBytes == 1) {
                // There was only one byte to read. Padding of two.
                byteToWrite2 = ((readByte1 & 0x3U) << 4U);
                paddingCount = 2U;
            } else {       // More than 1 remaining byte.
                // A second input byte (at least) is present.
                // The second output byte can sure be written.
                readByte2 = static_cast<unsigned char>(inputStream.readByte());
                byteToWrite2 = ((readByte1 & 0x3U) << 4U) | ((readByte2 & 0xf0U) >> 4U);
            }

            if (remainingBytes == 2) {
                // There was only two bytes to read.
                // Padding of one.
                byteToWrite3 = ((readByte2 & 0xfU) << 2U);
                paddingCount = 1U;
            } else if (remainingBytes > 2) {
                // A third input byte is present.
                // The third and fourth byte can be written.
                readByte3 = static_cast<unsigned char>(inputStream.readByte());
                byteToWrite3 = ((readByte2 & 0xfU) << 2U) | ((readByte3 & 0xc0U) >> 6U);
                byteToWrite4 = (readByte3 & 0x3fU);
                paddingCount = 0U;
            }
        }

        // Encodes the four bytes, if no error occurred.
        if (mustContinue) { // Note: cast to "char", else ARM build fails.
            success = success && outputStream.writeByte(static_cast<char>(base64Index[byteToWrite1])); // NOLINT(*-pro-bounds-constant-array-index)
            success = success && outputStream.writeByte(static_cast<char>(base64Index[byteToWrite2])); // NOLINT(*-pro-bounds-constant-array-index)
            success = success && outputStream.writeByte(static_cast<char>(paddingCount == 2U ? paddingChar : base64Index[byteToWrite3])); // NOLINT(*-pro-bounds-constant-array-index)
            success = success && outputStream.writeByte(static_cast<char>(paddingCount >= 1U ? paddingChar : base64Index[byteToWrite4])); // NOLINT(*-pro-bounds-constant-array-index)
        } else {
            mustContinue = false;
        }
    }

    return success;
}

bool Base64Util::decodeFromBase64(juce::InputStream& inputStream, juce::OutputStream& outputStream) noexcept
{
    jassert(paddingIndex == 64U);
    jassert(charToIndex[paddingChar] == paddingIndex);// NOLINT(clion-misra-cpp2008-5-0-4)

    // The remaining input size must be a multiple of 4! It must also be valid, else we won't know when to stop.
    const auto remainingBytes = inputStream.getNumBytesRemaining();
    if ((remainingBytes < 0) || ((remainingBytes % 4) != 0)) {
        return false;
    }

    // If there is nothing to do, do nothing!
    if (remainingBytes == 0) {
        return true;
    }

    auto success = true;

    while (inputStream.getNumBytesRemaining() > 0) {

        auto padding = 0;

        // Reads at least the two first ascii characters.
        const auto readByte1 = static_cast<unsigned char>(inputStream.readByte());
        const auto readByte2 = static_cast<unsigned char>(inputStream.readByte());
        auto readByte3 = static_cast<unsigned char>(inputStream.readByte());
        auto readByte4 = static_cast<unsigned char>(inputStream.readByte());

        if (readByte3 == paddingChar) {     // NOLINT(clion-misra-cpp2008-5-0-4)
            if (readByte4 != paddingChar) {     // If the third byte is padding, it implies the fourth should also be!  // NOLINT(clion-misra-cpp2008-5-0-4)
                success = false;
                break;
            }
            readByte3 = base64Index[0U];
            ++padding;
        }
        if (readByte4 == paddingChar) {     // NOLINT(clion-misra-cpp2008-5-0-4)
            readByte4 = base64Index[0U];
            ++padding;
        }

        // Converts them to Base64 indexes.
        const auto indexByte1 = charToIndex[readByte1];
        const auto indexByte2 = charToIndex[readByte2];
        const auto indexByte3 = charToIndex[readByte3];
        const auto indexByte4 = charToIndex[readByte4];

        // Checks the limit, and also the possible padding. Also, Byte 1 and 2 can not be paddings.
        if ((indexByte1 == 255) || (indexByte2 == 255) || (indexByte3 == 255) || (indexByte4 == 255)     // NOLINT(clion-misra-cpp2008-5-0-4)
            || (indexByte1 == paddingIndex) || (indexByte2 == paddingIndex))     // NOLINT(clion-misra-cpp2008-5-0-4)
        {
            success = false;
            break;
        }

        // The first output byte can always be written.
        auto writeByte = (indexByte1 << 2U) | ((indexByte2 >> 4U) & 0x3);     // NOLINT(clion-misra-cpp2008-5-0-4, *-signed-bitwise)
        outputStream.writeByte(static_cast<char>(writeByte));

        // Stops here if there is a padding of 2.
        if (padding < 2) {
            // There is padding. Padding to 1 or less, so the second character can be extracted.
            writeByte = ((indexByte2 & 0xf) << 4) | ((indexByte3 >> 2) & 0xf);     // NOLINT(clion-misra-cpp2008-5-0-4, *-signed-bitwise)
            outputStream.writeByte(static_cast<char>(writeByte));

            if (padding == 0) {     // The third (and last) character can be extracted.
                writeByte = ((indexByte3 & 0x3) << 6) | (indexByte4 & 0x3f);     // NOLINT(clion-misra-cpp2008-5-0-4, *-signed-bitwise)
                outputStream.writeByte(static_cast<char>(writeByte));
            }
        }
    }

    return success;
}

std::vector<unsigned char> Base64Util::decodeBase64StringToVector(const juce::String &inputText, bool &success) noexcept
{
    success = true;

    // Converts the text from base64.
    juce::MemoryOutputStream memoryOutputStreamForString;
    memoryOutputStreamForString << inputText;
    memoryOutputStreamForString.flush();
    // We now have our Input Stream.
    juce::MemoryInputStream inputStream(memoryOutputStreamForString.getMemoryBlock(), true);      // Strange, if don't keep copy, doesn't work.
    inputStream.setPosition(0);

    // Output Stream where the result is put.
    juce::MemoryOutputStream outputStream;

    // Decodes the Base64.
    success = decodeFromBase64(inputStream, outputStream);
    if (!success) {
        return { };
    }

    auto memoryBlock = outputStream.getMemoryBlock();
    const auto size = memoryBlock.getSize();

    // Generates the array.
    std::vector<unsigned char> result;
    for (size_t index = 0U; index < size; ++index) {
        result.push_back(static_cast<unsigned char>(memoryBlock[index]));
    }

    return result;
}

juce::String Base64Util::encodeToBase64(const juce::String& inputText, bool& success) noexcept
{
    // Source to InputStream.
    const juce::MemoryBlock sourceMemoryBlock(inputText.toUTF8(), static_cast<size_t>(inputText.length()));
    juce::MemoryInputStream inputStream(sourceMemoryBlock, true);

    juce::MemoryOutputStream outputStream;
    success = encodeToBase64(inputStream, outputStream);
    if (!success) {
        jassertfalse;
        return { };
    }

    return MemoryBlockUtil::extractString(outputStream.getMemoryBlock(), 0, static_cast<juce::int64>(outputStream.getDataSize()), success, false);
}

}   // namespace arkostracker
