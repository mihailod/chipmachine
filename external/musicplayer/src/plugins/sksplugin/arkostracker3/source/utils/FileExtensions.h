#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/**
 * Little class holding the extensions of various files.
 */
class FileExtensions
{
public:
    /** Prevents instantiation. */
    FileExtensions() = delete;

    static const juce::String instrumentExtension;                          // The extension of the Instrument file, with a comma.
    static const juce::String instrumentExtensionWithWildcard;
    static const juce::String instrumentExtensionsFilter;                   // The extensions of the Instrument files.

    static const juce::String arpeggioExtensionWithWildcard;
    static const juce::String pitchExtensionWithWildcard;

    static const juce::String wavExtensionWithWildcard;
    static const juce::String txtExtensionWithWildcard;

    static const juce::String loadSongExtensionsFilter;                     // A String containing all the extensions to load from.
    static const juce::String saveSongExtension;                            // The extension of the saved Song.
    static const juce::String saveSongExtensionFilter;                      // The extension filter of the saved Song.

    static const juce::String ymExtensionWithoutDot;
    static const juce::String ymExtensionWithWildcard;

    static const juce::String vgmExtensionWithoutDot;
    static const juce::String vgzExtensionWithoutDot;

    static const juce::String akgExtensionWithoutDot;
    static const juce::String akyExtensionWithoutDot;
    static const juce::String akmExtensionWithoutDot;
    static const juce::String akxExtensionWithoutDot;
    static const juce::String fapExtensionWithoutDot;
    static const juce::String midiExtensionWithoutDot;
    static const juce::String modExtensionWithoutDot;

    static const juce::String wavExtensionWithoutDot;

    static const juce::String xmlExtensionWithoutDot;
    static const juce::String xmlExtensionFilter;

    static const juce::String csvExtensionWithWildcard;

    static const juce::String dskExtensionWithoutDot;
    static const juce::String dskExtensionWithWildcard;
    static const juce::String snaExtensionWithoutDot;
};

}   // namespace arkostracker
