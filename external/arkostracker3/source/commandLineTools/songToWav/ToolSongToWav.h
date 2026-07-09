#pragma once

#include <juce_core/juce_core.h>

#include "../utils/CommandLineArgumentDescriptor.h"

namespace arkostracker
{

/** Tool that loads any Song that AT can handle, and converts it to WAV. */
class ToolSongToWav
{
public:
    static int execute(int argc, char* argv[]);     // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)

private:
    static const juce::String parameterMixingFlat;
    static const juce::String parameterMixingCpc;
    static const juce::String parameterMixingCustom;

    static bool readMixingOptions(const Parameter& parameterMixing, int& volumeLeft, int& volumeCenter, int& volumeRight) noexcept;
};

}   // namespace arkostracker
