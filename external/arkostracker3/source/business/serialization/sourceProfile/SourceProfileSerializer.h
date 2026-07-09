#pragma once

#include "juce_core/juce_core.h"

#include "../../sourceProfile/SourceProfile.h"

namespace arkostracker 
{

class SourceProfile;

/** Serializes and deserializes the SourceProfile into XML to use with the Preferences. */
class SourceProfileSerializer
{
public:
    /** Prevents instantiation. */
    SourceProfileSerializer() = delete;

    /**
     * Serializes SourceProfiles into XML.
     * @param sourceProfiles the SourceProfiles.
     * @return the node. Never null or empty.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<SourceProfile>& sourceProfiles) noexcept;

    /**
     * Deserializes SourceProfiles from XML.
     * @param sourceProfilesNode the SourceProfiles node ("sourceProfiles").
     * @return the SourceProfiles.
     */
    static std::vector<SourceProfile> deserialize(const juce::XmlElement& sourceProfilesNode) noexcept;

    /**
     * Deserializes a SourceProfile from XML.
     * @param sourceProfileNode the SourceProfile node ("sourceProfile").
     * @return the SourceProfile.
     */
    static SourceProfile deserializeOne(const juce::XmlElement& sourceProfileNode) noexcept;

    /**
     * Serializes a SourceProfile into XML.
     * @param sourceProfile the SourceProfile.
     * @return the node. Never null or empty.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const SourceProfile& sourceProfile) noexcept;
};

}   // namespace arkostracker
