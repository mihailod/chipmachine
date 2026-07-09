#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Constants to the tags defining Areas. */
class AreaTag
{
public:
    static const juce::String tagCodeRegion;
    static const juce::String tagByteRegion;
    static const juce::String tagWordRegion;
    static const juce::String tagPointerRegion;
    static const juce::String tagForceReferenceArea;
    static const juce::String tagForceNonReferenceArea;

    static const juce::String tagStart;
    static const juce::String tagEnd;

    static const juce::String tagCodeRegionStart;
    static const juce::String tagCodeRegionEnd;
    static const juce::String tagByteRegionStart;
    static const juce::String tagByteRegionEnd;
    static const juce::String tagWordRegionStart;
    static const juce::String tagWordRegionEnd;
    static const juce::String tagPointerRegionStart;
    static const juce::String tagPointerRegionEnd;

    static const juce::String tagForceReferenceAreaStart;
    static const juce::String tagForceReferenceAreaEnd;
    static const juce::String tagForceNonReferenceAreaStart;
    static const juce::String tagForceNonReferenceAreaEnd;
    static const juce::String baseTagForceReferenceDuring;          // Followed by a digit (1, 2, 3, 4, etc.).
    static const juce::String baseTagForceNonReferenceDuring;       // Followed by a digit (1, 2, 3, 4, etc.).

    static const juce::String tagForceOneWordAreaWithForceReference;
    static const juce::String tagForceOneWordAreaWithForceNonReference;

    static const juce::String tagGenerateExternalLabel;
};

}   // namespace arkostracker
