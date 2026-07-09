#include "SaveStreamToFile.h"

#include "../../../../utils/FileUtil.h"

namespace arkostracker 
{

SaveStreamToFile::SaveStreamToFile(juce::File pOutputFile, std::unique_ptr<juce::InputStream> pStreamToSave, bool pZipped) :
        outputFile(std::move(pOutputFile)),
        streamToSave(std::move(pStreamToSave)),
        zipped(pZipped)
{
}

bool SaveStreamToFile::perform() noexcept
{
    // Deletes the file.
    outputFile.deleteFile();

    bool success;    // NOLINT(*-init-variables)

    if (zipped) {
        auto* inputStreamLocal = streamToSave.release();            // Release the object. Will be owned by the ZIP.
        success = FileUtil::saveToZip(outputFile, inputStreamLocal);
    } else {
        const auto length = streamToSave->getTotalLength();
        // Writes the InputStream into the Output Stream.
        juce::FileOutputStream fileOutputStream(outputFile);
        const auto writtenByteCount = fileOutputStream.writeFromInputStream(*streamToSave, -1);

        success = (writtenByteCount == length);
    }

    return success;
}

}   // namespace arkostracker
