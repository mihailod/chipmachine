#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Holds the name of the XML nodes for the source profiles. */
class SourceProfileXmlNodes
{
public:
    /** Prevents instantiation. */
    SourceProfileXmlNodes() = delete;

    static const juce::String sourceProfilesRoot;
    static const juce::String sourceProfileRoot;
    static const juce::String name;
    static const juce::String readOnly;
    static const juce::String littleEndian;
    static const juce::String generateComments;
    static const juce::String commentDeclaration;
    static const juce::String addressChangeDeclaration;
    static const juce::String addressDeclaration;
    static const juce::String byteDeclaration;
    static const juce::String wordDeclaration;
    static const juce::String labelDeclaration;
    static const juce::String stringDeclaration;
    static const juce::String constantDeclaration;
    static const juce::String binaryFileExtension;
    static const juce::String sourceFileExtension;
    static const juce::String tabulationLength;
};

}   // namespace arkostracker
