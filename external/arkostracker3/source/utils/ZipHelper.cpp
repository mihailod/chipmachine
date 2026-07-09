#include "ZipHelper.h"

#include "MemoryBlockUtil.h"
#include "../../thirdParty/lzh/LzhStSound.h"

namespace arkostracker 
{

std::unique_ptr<juce::InputStream> ZipHelper::unzip(juce::InputStream& inputStream) noexcept
{
    auto success = inputStream.setPosition(0);     // Security.
    jassert(success); (void)success;

    auto unzippedStream = std::unique_ptr<juce::InputStream>();

    // Tests first as a ZIP file.
    const auto zipFile = std::make_unique<juce::ZipFile>(&inputStream, false);
    const auto entryCount = zipFile->getNumEntries();
    jassert(entryCount <= 1);       // If more than one entry, we consider there is an error. 0 if not a ZIP.
    if (entryCount == 1) {
        const auto zipStream = std::unique_ptr<juce::InputStream>(zipFile->createStreamForEntry(0));
        if (zipStream != nullptr) {
            auto data = MemoryBlockUtil::fromInputStream(*zipStream);
            unzippedStream = std::make_unique<juce::MemoryInputStream>(data, true);
        }
    }

    if (unzippedStream == nullptr) {
        // Not a ZIP file. Maybe a GZIP?
        unzippedStream = depackGzip(inputStream);
    }

    if (unzippedStream == nullptr) {
        // Maybe a LHA?
        unzippedStream = depackLha(inputStream);
    }

    if (unzippedStream == nullptr) {
        // Not a compressed file. The Stream must be reset.
        success = inputStream.setPosition(0);
        jassert(success); (void)success;
    }

    return unzippedStream;
}

juce::MemoryBlock ZipHelper::unzip(const juce::MemoryBlock& inputMemoryBlock) noexcept
{
    juce::MemoryInputStream ymInputStream(inputMemoryBlock, false);
    const auto unzippedYmInputStream = unzip(ymInputStream);
    if (unzippedYmInputStream == nullptr) {
        // Not zipped.
        return inputMemoryBlock;
    }
    // Zipped.
    return MemoryBlockUtil::fromInputStream(*unzippedYmInputStream);
}

std::unique_ptr<juce::InputStream> ZipHelper::depackGzip(juce::InputStream& inputStream) noexcept
{
    auto unzippedStream = std::unique_ptr<juce::InputStream>();

    auto success = inputStream.setPosition(0);
    jassert(success); (void)success;

    juce::GZIPDecompressorInputStream gZipInputStream(&inputStream, false, juce::GZIPDecompressorInputStream::Format::gzipFormat);

    if (!gZipInputStream.isExhausted()) {
        // This is *perhaps* a GZIP Stream. Un-compresses it (not very efficient, but is there any other way?).
        juce::MemoryOutputStream memoryOutputStream;
        memoryOutputStream << gZipInputStream;

        success = (memoryOutputStream.getDataSize() > 0U);
        if (success) {
            // Opens an InputStream.
            unzippedStream = std::make_unique<juce::MemoryInputStream>(memoryOutputStream.getData(),
                                                                       memoryOutputStream.getDataSize(),
                                                                       true); // A copy is made, as the MemoryOutputStream will be destroyed.
        }
    }

    return unzippedStream;
}

std::unique_ptr<juce::InputStream> ZipHelper::depackLha(juce::InputStream& inputStream) noexcept
{
    // See https://github.com/jca02266/lha/blob/master/header.doc.md
    // In the end, the LHA lib must handle a MemoryBlock, but to check whether it's really a LHA, we
    // work on the original InputStream, to avoid duplicating the data into a MemoryBlock.
    constexpr size_t lengthIdTag = 5U;

    const auto success = inputStream.setPosition(0);
    jassert(success); (void)success;

    inputStream.skipNextBytes(1 + 1);       // Skips header size/sum.

    // Checks for the tag. If not present, the file is probably not packed.
    std::array<char, lengthIdTag> tagBuffer;                    // NOLINT(*-init-variables, *-member-init)
    inputStream.read(tagBuffer.data(), lengthIdTag);
    const juce::String readTag(tagBuffer.data(), lengthIdTag);
    if (readTag != "-lh5-") {
        // Not packed.
        return nullptr;
    }

    // Reads the sizes (little endian).
    const auto packedSize = static_cast<size_t>(inputStream.readInt());
    const auto unpackedSize = static_cast<size_t>(inputStream.readInt());
    if (unpackedSize < packedSize) {
        jassertfalse;
        return nullptr;         // Abnormal.
    }

    // Skips time, date, attribute.
    inputStream.skipNextBytes(2 + 2 + 1);

    // Checks the compression level. Must be 0.
    if (inputStream.readByte() != 0) {
        return nullptr;
    }

    // Reads the name length, skips it and a CRC.
    const auto nameLength = static_cast<unsigned char>(inputStream.readByte());
    inputStream.skipNextBytes(nameLength + 2U);

    // We know the format is correct. Now creates MemoryBlocks for both source and destination.
    // Copies the packed source into a MemoryBlock. Skips the header we have already read.
    juce::MemoryBlock source;
    const auto readBytes = inputStream.readIntoMemoryBlock(source);
    if (readBytes < packedSize) {
        // Couldn't read the whole file.
        // Strange however, it seems the files are bigger of 1 byte than necessary.
        jassertfalse;
        return nullptr;
    }

    // Depacks.
    lzh::CLzhDepacker depacker{};
    juce::MemoryBlock destination(unpackedSize, false);
    depacker.LzUnpack(source.getData(), static_cast<int>(packedSize),   // NOLINT(clion-misra-cpp2008-5-2-8)
                      destination.getData(), static_cast<int>(destination.getSize()));

    return std::make_unique<juce::MemoryInputStream>(destination, true);
}

}   // namespace arkostracker
