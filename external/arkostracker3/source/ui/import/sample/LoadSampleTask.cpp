#include "LoadSampleTask.h"

namespace arkostracker
{

LoadSampleTask::LoadSampleTask(juce::String pFileToLoad) noexcept :
        fileToLoad(std::move(pFileToLoad))
{
}

juce::String LoadSampleTask::getErrorMessage(const SampleLoader::Outcome outcome) noexcept
{
    switch (outcome) {
        default:
        case SampleLoader::Outcome::success:
            jassertfalse;           // Not supposed to happen!
            return { };
        case SampleLoader::Outcome::fileNotFound:
            return juce::translate("File does not exist!");
        case SampleLoader::Outcome::moreThan128k:
            return juce::translate("Samples can only be 128k long max!");
        case SampleLoader::Outcome::formatUnknown:
            return juce::translate("Unknown file format!");
        case SampleLoader::Outcome::parsingError:
            return juce::translate("Unable to parse the file data!");
    }
}


// Task method implementations.
// ===================================================

std::pair<bool, std::unique_ptr<SampleLoader::Result>> LoadSampleTask::performTask() noexcept
{
    auto result = SampleLoader::readFile(fileToLoad);
    return std::make_pair(true, std::move(result));     // Always returns true, else the task returns nullptr, which we don't want to keep the reason why.
}

}   // namespace arkostracker
