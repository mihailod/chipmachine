#include "FileUtil.h"

#include "NumberUtil.h"
#include "StringUtil.h"
#include "ZipHelper.h"

namespace arkostracker 
{
juce::File FileUtil::getFileFromString(const juce::String& inputFileString) noexcept
{
    return juce::File::getCurrentWorkingDirectory().getChildFile(inputFileString);    // To return an absolute path, else JUCE assertion raises.
}

bool FileUtil::writeXmlToFile(const juce::XmlElement& xmlElement, const juce::File& outputFile, const bool zip) noexcept
{
    if (!zip) {
        return xmlElement.writeTo(outputFile);      // This takes care of replacing the original file.
    }

    // Zips the file.

    // First, deletes the output file. It must succeed.
    const auto success = outputFile.deleteFile();
    if (!success) {
        return false;
    }

    // Writes to buffer.
    juce::MemoryOutputStream outputStream;
    xmlElement.writeTo(outputStream);

    // Creates a reader to the buffer.
    // Important: a copy is made because the output stream will be deleted by the time the zip builder starts to work.
    auto* inputStream = new juce::MemoryInputStream(outputStream.getData(), outputStream.getDataSize(), true);  // Managed by the ZipBuilder.

    // Zip the buffer.
    return saveToZip(outputFile, inputStream);
}

bool FileUtil::saveToZip(const juce::File& outputFile, juce::InputStream* streamToZip) noexcept
{
    // Zip the buffer (maximum compression).
    juce::ZipFile::Builder zipBuilder;
    zipBuilder.addEntry(streamToZip, 9, outputFile.getFileName(), juce::Time::getCurrentTime());

    // Writes the zip to the output file.
    juce::FileOutputStream fileOutputStream(outputFile);
    return zipBuilder.writeToStream(fileOutputStream, nullptr);
}

std::unique_ptr<juce::InputStream> FileUtil::openFileOrZip(const juce::File& file) noexcept
{
    if (!file.existsAsFile()) {
        jassertfalse;       // File doesn't exist.
        return nullptr;
    }

    juce::FileInputStream fis(file);
    // Was it a zip? If yes, uses the unzip stream.
    if (auto unzippedStream = ZipHelper::unzip(fis); unzippedStream != nullptr) {
        return unzippedStream;
    }
    // Else uses the raw stream from the file, which is not a zip.
    return std::make_unique<juce::FileInputStream>(file);
}

bool FileUtil::saveMemoryBlockToFile(const juce::String& fileToCreate, const juce::MemoryBlock& memoryBlockToSave) noexcept
{
    const auto file = getFileFromString(fileToCreate);
    return saveMemoryBlockToFile(file, memoryBlockToSave);
}

bool FileUtil::saveMemoryBlockToFile(const juce::File& file, const juce::MemoryBlock& memoryBlockToSave) noexcept
{
    (void)file.deleteFile();

    juce::FileOutputStream outputStream(file);
    return outputStream.write(memoryBlockToSave.getData(), memoryBlockToSave.getSize());
}

std::unique_ptr<juce::MemoryOutputStream> FileUtil::gZip(const void* data, const size_t dataSize) noexcept
{
    auto compressedData = std::make_unique<juce::MemoryOutputStream>();
    auto gzip = juce::GZIPCompressorOutputStream(*compressedData, 9,
                                                 juce::GZIPCompressorOutputStream::WindowBitsValues::windowBitsGZIP);   // Flags required for VGM.
    const auto success = gzip.write(data, dataSize);
    jassert(success); (void)success;

    gzip.flush();

    return compressedData;
}

juce::File FileUtil::buildFileWithSuffixAndExtension(const juce::File& inputFile, const juce::String& suffix, const juce::String& extensionWithoutDot) noexcept
{
    const auto newFileName = inputFile.getFileNameWithoutExtension() + suffix + "." + extensionWithoutDot;
    const auto newPath = inputFile.getParentDirectory().getFullPathName() + juce::File::getSeparatorChar() + newFileName;

    return { newPath };
}

juce::File FileUtil::buildFileWithSuffix(const juce::File& inputFile, const juce::String& suffix) noexcept
{
    return inputFile.getParentDirectory().getFullPathName() + juce::File::getSeparatorChar() +
        inputFile.getFileNameWithoutExtension() + suffix + inputFile.getFileExtension();
}

juce::File FileUtil::getFileWithoutCopyIndex(const juce::File& inputFile)
{
    const auto pathAndFileWithoutExtension = inputFile.getParentDirectory().getFullPathName() + "/" + inputFile.getFileNameWithoutExtension()
        .trimCharactersAtEnd(" ");

    // Finds the last open/close parenthesis.
    if (!pathAndFileWithoutExtension.endsWithChar(')')) {
        return inputFile;
    }
    const auto openParenthesisIndex = pathAndFileWithoutExtension.lastIndexOfChar('(');
    if (openParenthesisIndex < 0) {
        return inputFile;
    }

    // Grabs what is between. Is it a number?
    const auto betweenParenthesisString = pathAndFileWithoutExtension.substring(openParenthesisIndex + 1, pathAndFileWithoutExtension.length() - 1);
    auto success = true;
    const auto readIndex = StringUtil::stringToInt(betweenParenthesisString, success);
    if (!success || readIndex < 0) {
        return inputFile;
    }

    // Everything is fine. Removes the last parenthesis, trimmed, and puts back the extension.
    auto finalPath = pathAndFileWithoutExtension.substring(0, openParenthesisIndex).trimCharactersAtEnd(" ") + inputFile.getFileExtension();

    return finalPath;
}

}   // namespace arkostracker
