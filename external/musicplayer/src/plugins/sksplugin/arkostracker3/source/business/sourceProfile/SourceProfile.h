#pragma once

#include "../../export/sourceGenerator/SourceGeneratorConfiguration.h"

namespace arkostracker 
{

/**
 * A source profile is only a Source Configuration, along with a human-readable name ("Z80 with comments" for example), and a flag indicating whether
 * the profile is read-only (for templates) or a user one.
 */
class SourceProfile
{
public:
    /**
        Constructor.
        @param sourceGeneratorConfiguration the configuration.
        @param name the name of the profile ("Z80 with comments" for example).
        @param readOnly true if the profile is a template (read-only). False if from the user.
    */
    SourceProfile(SourceGeneratorConfiguration sourceGeneratorConfiguration, juce::String name, bool readOnly) noexcept;

    /**
     * Constructor from another profile.
     * @param sourceProfile the original source profile.
     * @param newName the name of the new profile.
     * @param readOnly true if the profile is a template (read-only). False if from the user.
     */
    SourceProfile(const SourceProfile& sourceProfile, juce::String newName, bool readOnly = false) noexcept;

    /** @return the configuration. */
    const SourceGeneratorConfiguration& getSourceGeneratorConfiguration() const noexcept;
    /** @return the human-readable name of the profile. */
    juce::String getName() const noexcept;
    /** @return true if the profile is a template (read-only). False if from the user.*/
    bool isReadOnly() const noexcept;

private:
    SourceGeneratorConfiguration sourceGeneratorConfiguration;                      // The configuration.
    juce::String name;                                                              // The name of the profile ("Z80 with comments" for example).
    bool readOnly;                                                                  // True if the profile is a template (read-only). False if from the user.
};

}   // namespace arkostracker
