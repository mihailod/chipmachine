#pragma once

#include <memory>
#include <vector>

#include <juce_core/juce_core.h>

#include "../../../song/subsong/Pattern.h"

namespace arkostracker
{

/** Serializes the Patterns. */
class PatternSerializer
{
public:
    /** Prevents instantiation. */
    PatternSerializer() = delete;

    /**
     * Serializes to XML the given Pattern.
     * @param pattern the object to serialize.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const Pattern& pattern) noexcept;

    /**
     * Serializes to XML the given Patterns.
     * @param patterns the objects to serialize.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<Pattern>& patterns) noexcept;

    /**
     * Deserializes an XML into Patterns.
     * @param patternsRoot the XML Element which contains all the Pattern nodes.
     * @return the Patterns, or empty if an error occurred.
     */
    static std::vector<Pattern> deserializePatterns(const juce::XmlElement& patternsRoot) noexcept;

    /**
     * Deserializes an XML into Patterns.
     * @param patternNode the XML Element which contains a Pattern node.
     * @return the Pattern, or nullptr if an error occurred.
     */
    static std::unique_ptr<Pattern> deserializePattern(const juce::XmlElement& patternNode) noexcept;

private:
    /**
     * Encodes a track index (track index and possible linked one).
     * @param rootXml where to add the nodes.
     * @param trackIndexAndLinkedTrackIndex the track ids.
     * @param nodeName the node to create, which will contain the unlink/link nodes.
     */
    static void addTrackIndexNode(juce::XmlElement& rootXml, const Pattern::TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex, const juce::String& nodeName) noexcept;

    static Pattern::TrackIndexAndLinkedTrackIndex readTrackIndexes(const juce::XmlElement* xmlNode) noexcept;

    static const juce::String nodePatterns;
    static const juce::String nodePattern;
    static const juce::String nodeColorArgb;
    static const juce::String nodeTrackIndexes;
    static const juce::String nodeTrackIndex;
    static const juce::String nodeLinkedTrackIndex;
    static const juce::String nodeSpeedTrackIndex;
    static const juce::String nodeEventTrackIndex;
};

}   // namespace arkostracker
