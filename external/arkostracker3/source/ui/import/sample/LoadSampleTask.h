#pragma once

#include <juce_core/juce_core.h>

#include "../../../business/instrument/SampleLoader.h"
#include "../../../utils/task/Task.h"

namespace arkostracker
{

/** Task to load a sample. */
class LoadSampleTask : public Task<std::unique_ptr<SampleLoader::Result>>
{
public:
    /**
     * Constructor.
     * @param fileToLoad the file (WAV, etc.). May not exist.
     */
    explicit LoadSampleTask(juce::String fileToLoad) noexcept;

    /**
     * @return a displayable error message from the task outcome.
     * @param outcome how the task went.
     */
    static juce::String getErrorMessage(SampleLoader::Outcome outcome) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<SampleLoader::Result>> performTask() noexcept override;

private:
    juce::String fileToLoad;
};

}   // namespace arkostracker
