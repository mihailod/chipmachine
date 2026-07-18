#include "PatternSerializer.h"

#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String PatternSerializer::nodePatterns = "patterns";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodePattern = "pattern";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeColorArgb = "colorArgb";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeTrackIndexes = "trackIndexes";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeTrackIndex = "trackIndex";                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeLinkedTrackIndex = "linkedTrackIndex";        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeSpeedTrackIndex = "speedTrackIndex";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PatternSerializer::nodeEventTrackIndex = "eventTrackIndex";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> PatternSerializer::serialize(const Pattern& pattern) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodePattern);

    const auto channelCount = pattern.getChannelCount();
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        const auto trackIndexAndLinkedTrackIndex = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
        addTrackIndexNode(*rootXml, trackIndexAndLinkedTrackIndex, nodeTrackIndexes);
    }

    addTrackIndexNode(*rootXml, pattern.getSpecialTrackIndexAndLinkedTrackIndex(true), nodeSpeedTrackIndex);
    addTrackIndexNode(*rootXml, pattern.getSpecialTrackIndexAndLinkedTrackIndex(false), nodeEventTrackIndex);
    XmlHelper::addValueNode(*rootXml, nodeColorArgb, pattern.getArgbColor());

    return rootXml;
}

void PatternSerializer::addTrackIndexNode(juce::XmlElement& rootXml, const Pattern::TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex, const juce::String& nodeName) noexcept
{
    auto& indexNode = XmlHelper::addNode(rootXml, nodeName);

    XmlHelper::addValueNode(indexNode, nodeTrackIndex, trackIndexAndLinkedTrackIndex.getUnlinkedTrackIndex());
    // Encodes the linked track index if present.
    if (trackIndexAndLinkedTrackIndex.getLinkedTrackIndex().isPresent()) {
        XmlHelper::addValueNode(indexNode, nodeLinkedTrackIndex, trackIndexAndLinkedTrackIndex.getLinkedTrackIndex().getValue());
    }
}

std::unique_ptr<juce::XmlElement> PatternSerializer::serialize(const std::vector<Pattern>& patterns) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodePatterns);

    // As soon as an error occurs, returns an empty pointer.
    for (const auto& pattern : patterns) {
        auto xmlPtr = serialize(pattern);
        if (xmlPtr == nullptr) {
            jassertfalse;       // Serialization failed.
            return nullptr;
        }

        rootXml->addChildElement(xmlPtr.release());
    }

    return rootXml;
}

std::vector<Pattern> PatternSerializer::deserializePatterns(const juce::XmlElement& patternsRoot) noexcept
{
    std::vector<Pattern> patterns;

    // Gets every Pattern node.
    for (const auto* patternNode : XmlHelper::getChildrenList(patternsRoot, nodePattern)) {
        auto pattern = deserializePattern(*patternNode);
        if (pattern == nullptr) {
            return {};              // Stops as soon as an error is detected.
        }
        patterns.emplace_back(*pattern);
    }

    return patterns;
}

std::unique_ptr<Pattern> PatternSerializer::deserializePattern(const juce::XmlElement& patternNode) noexcept
{
    const auto colorArgb = XmlHelper::readUnsignedInt(patternNode, nodeColorArgb, LookAndFeelConstants::defaultPatternColor);

    // Gets every Track indexes (unlink and possibly linked).
    Pattern pattern(colorArgb);
    for (const auto* trackIndexesNode : XmlHelper::getChildrenList(patternNode, nodeTrackIndexes)) {
        const auto trackIndexes = readTrackIndexes(trackIndexesNode);
        if (!trackIndexes.isValid()) {
            jassertfalse;       // Invalid track index.
            return nullptr;
        }
        pattern.addTrackIndex(trackIndexes);
    }
    if (pattern.getChannelCount() == 0) {
        jassertfalse;       // No track present!
        return nullptr;
    }

    const auto speedTrackIndexes = readTrackIndexes(patternNode.getChildByName(nodeSpeedTrackIndex));
    const auto eventTrackIndexes = readTrackIndexes(patternNode.getChildByName(nodeEventTrackIndex));
    if (!speedTrackIndexes.isValid() || !eventTrackIndexes.isValid()) {
        jassertfalse;       // Invalid special track index.
        return nullptr;
    }
    pattern.setSpecialTrackIndex(true, speedTrackIndexes);
    pattern.setSpecialTrackIndex(false, eventTrackIndexes);

    return std::make_unique<Pattern>(pattern);
}

Pattern::TrackIndexAndLinkedTrackIndex PatternSerializer::readTrackIndexes(const juce::XmlElement* xmlNode) noexcept
{
    if (xmlNode == nullptr) {
        jassertfalse;       // Not present? Abnormal.
        return Pattern::TrackIndexAndLinkedTrackIndex(-1, -1);
    }

    const auto trackIndex = XmlHelper::readInt(*xmlNode, nodeTrackIndex, -1);
    const auto linkedTrackIndex = XmlHelper::readInt(*xmlNode, nodeLinkedTrackIndex, -1);

    return Pattern::TrackIndexAndLinkedTrackIndex(trackIndex, (linkedTrackIndex < 0) ? OptionalInt() : linkedTrackIndex);
}

}   // namespace arkostracker
