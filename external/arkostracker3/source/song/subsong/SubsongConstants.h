#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Some constants in the Subsongs. */
class SubsongConstants
{
public:
    /** Prevents instantiation. */
    SubsongConstants() = delete;

    static const int minimumSpeed;
    static const int maximumSpeed;
    static const int defaultSpeed;

    /** Starting at 0. */
    static const int defaultDigiChannel;

    static const int defaultPrimaryHighlight;
    static const int defaultSecondaryHighlight;

    static const juce::String defaultFirstSubsongName;
};

}   // namespace arkostracker
