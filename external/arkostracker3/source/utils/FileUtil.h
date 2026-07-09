#pragma once

#include <memory>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Utility class for files. */
class FileUtil
{
public:
    /** Prevents instantiation. */
    FileUtil() = delete;

    /**
     * @return a File from the given file path. It uses a special code, else JUCE asserts.
     * @param filePath the path to a file.
     */
    static juce::File getFileFromString(const juce::String& filePath) noexcept;

    /**
     * Writes an XML Element into a file, zipped or not, overwriting it.
     * @param xmlElement the XML element.
     * @param outputFile the output file. It may exit, but will be deleted first.
     * @param zip true to zip the file.
     * @return true if successful.
     */
    static bool writeXmlToFile(const juce::XmlElement& xmlElement, const juce::File& outputFile, bool zip) noexcept;

    /**
     * Opens a file (plain or zip) and returns the stream of data (unpacked if zip).
     * @param file the file to open.
     * @return the stream of data, or nullptr if an error occurred.
     */
    static std::unique_ptr<juce::InputStream> openFileOrZip(const juce::File& file) noexcept;

    /**
     * Saves the content of a MemoryBlock to a file.
     * @param fileToCreate the file. If existing, it is overwritten.
     * @param memoryBlockToSave the MemoryBlock to save.
     */
    static bool saveMemoryBlockToFile(const juce::String& fileToCreate, const juce::MemoryBlock& memoryBlockToSave) noexcept;

    /**
     * Saves the content of a MemoryBlock to a file.
     * @param file the file. It is deleted first.
     * @param memoryBlockToSave the MemoryBlock to save.
     * @return true if everything went file.
     */
    static bool saveMemoryBlockToFile(const juce::File& file, const juce::MemoryBlock& memoryBlockToSave) noexcept;

    /**
     * Saves a Stream into a ZIP file.
     * @param outputFile the file to create. If present, will be overwritten.
     * @param streamToZipDeletedByZip the data to zip. Warning, will be owned by this method, don't try to release it!
     * @return true if everything went file.
     */
    static bool saveToZip(const juce::File& outputFile, juce::InputStream* streamToZipDeletedByZip) noexcept;

    /**
     * Compresses the given data into a GZip stream.
     * @param data the data.
     * @param dataSize the size.
     * @return the stream.
     */
    static std::unique_ptr<juce::MemoryOutputStream> gZip(const void* data, size_t dataSize) noexcept;

    /**
     * @return a new File with a possible suffix after the file name, and a new extension.
     * @param inputFile the input file.
     * @param suffix the suffix ("_playerconfig" for example). May be empty.
     * @param extensionWithoutDot the new extension ("bin" for example).
     */
    static juce::File buildFileWithSuffixAndExtension(const juce::File& inputFile, const juce::String& suffix, const juce::String& extensionWithoutDot) noexcept;

    /**
     * @return a new File with a possible suffix after the file name and before the extension.
     * @param inputFile the input file.
     * @param suffix the suffix ("_playerconfig" for example). May be empty.
     */
    static juce::File buildFileWithSuffix(const juce::File& inputFile, const juce::String& suffix) noexcept;

    /**
     * @return the file without the copy index. For example, "/stuff/bob (1).txt" will return "/stuff/bob.txt". Parenthesis without number at the end "(hello)"
     * will not be removed.
     */
    static juce::File getFileWithoutCopyIndex(const juce::File& inputFile);
};

}   // namespace arkostracker
