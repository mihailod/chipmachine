#pragma once

#include <utility>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Saves an InputStream to a file, zipped if wanted. */
class SaveStreamToFile
{
public:
    /**
     * Saves data to a File. It is overwritten if existing.
     * @param outputFile the output file. May exist.
     * @param streamToSave the stream to save.
     * @param zipped true to zip the file. If true, this does not change the extension of the file.
     */
    SaveStreamToFile(juce::File outputFile, std::unique_ptr<juce::InputStream> streamToSave, bool zipped = false);

    /**
     * Performs the operation.
     * @return true if success.
     */
    bool perform() noexcept;

private:
    const juce::File outputFile;
    std::unique_ptr<juce::InputStream> streamToSave;
    const bool zipped;
};

}   // namespace arkostracker
