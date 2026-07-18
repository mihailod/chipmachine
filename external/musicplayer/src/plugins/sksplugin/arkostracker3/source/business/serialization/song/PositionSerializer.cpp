#include "PositionSerializer.h"

#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String PositionSerializer::nodePositionsRoot("positions");            // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)

const juce::String PositionSerializer::nodePositionRoot("position");              // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)

const juce::String PositionSerializer::nodePatternIndex("patternIndex");              // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeHeight("height");                          // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeMarkerName("markerName");                  // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeMarkerColor("markerColor");                // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)

const juce::String PositionSerializer::nodeTranspositions("transpositions");                // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeTransposition("transposition");                // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeChannelWithTransposition("channel");                // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)
const juce::String PositionSerializer::nodeTranspositionValue("value");                // NOLINT(fuchsia-statically-constructed-objects,cert-err58-cpp)

std::unique_ptr<juce::XmlElement> PositionSerializer::serialize(const Position& position) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodePositionRoot);

    XmlHelper::addValueNode(*rootXml, nodePatternIndex, position.getPatternIndex());
    XmlHelper::addValueNode(*rootXml, nodeHeight, position.getHeight());
    XmlHelper::addValueNode(*rootXml, nodeMarkerName, position.getMarkerName());
    XmlHelper::addValueNode(*rootXml, nodeMarkerColor, position.getMarkerColor());

    // Encodes the transpositions.
    auto& xmlNodeTranspositions = XmlHelper::addNode(*rootXml, nodeTranspositions);

    const auto& channelToTransposition = position.getChannelToTransposition();
    for (const auto&[channelIndex, transpositionValue] : channelToTransposition) {
        jassert(transpositionValue != 0);           // 0 shouldn't be present in the original structure!
        if (transpositionValue != 0) {
            auto& xmlNodeTransposition = XmlHelper::addNode(xmlNodeTranspositions, nodeTransposition);

            XmlHelper::addValueNode(xmlNodeTransposition, nodeChannelWithTransposition, channelIndex);
            XmlHelper::addValueNode(xmlNodeTransposition, nodeTranspositionValue, transpositionValue);
        }
    }

    return rootXml;
}

std::unique_ptr<juce::XmlElement> PositionSerializer::serialize(const std::vector<Position>& positions) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(nodePositionsRoot);

    // As soon as an error occurs, returns an empty pointer.
    for (const auto& position : positions) {
        auto xmlPtr = serialize(position);
        if (xmlPtr == nullptr) {
            jassertfalse;       // Serialization failed.
            return nullptr;
        }

        rootXml->addChildElement(xmlPtr.release());
    }

    return rootXml;
}

std::unique_ptr<Position> PositionSerializer::deserializePosition(const juce::XmlElement& positionRootElement) noexcept
{
    auto patternIndex = XmlHelper::readInt(positionRootElement, nodePatternIndex, -1);
    auto height = XmlHelper::readInt(positionRootElement, nodeHeight, -1);
    auto markerName = XmlHelper::readString(positionRootElement, nodeMarkerName);
    auto markerColorArgb = XmlHelper::readUnsignedInt(positionRootElement, nodeMarkerColor, 0);     // Alpha cannot be 0.

    // Some basic check...
    if ((patternIndex < 0) || (height < 0) || (markerColorArgb == 0)) {
        return nullptr;
    }

    std::unordered_map<int, int> channelToTransposition;

    // Transpositions, if any?
    const auto* transpositionsNode = positionRootElement.getChildByName(nodeTranspositions);
    if (transpositionsNode != nullptr) {
        // Gets every transposition node.
        const auto* transpositionNode = transpositionsNode->getChildByName(nodeTransposition);
        while (transpositionNode != nullptr) {
            auto channelIndex = XmlHelper::readInt(*transpositionNode, nodeChannelWithTransposition, -1);
            constexpr auto transpositionFallback = -10000;      // We know this value can never be reached.
            auto transpositionValue = XmlHelper::readInt(*transpositionNode, nodeTranspositionValue, transpositionFallback);
            // Valid values?
            if ((channelIndex < 0 ) || (transpositionValue == transpositionFallback)) {
                return nullptr;
            }
            channelToTransposition.insert(std::make_pair(channelIndex, transpositionValue));

            // Next sibling.
            transpositionNode = transpositionNode->getNextElementWithTagName(nodeTransposition);
        }
    }

    return std::make_unique<Position>(patternIndex, height, markerName, markerColorArgb, channelToTransposition);
}

std::vector<Position> PositionSerializer::deserializePositions(const juce::XmlElement& positionsRoot) noexcept
{
    std::vector<Position> positions;

    // Gets every Position node.
    const auto* positionNode = positionsRoot.getChildByName(nodePositionRoot);
    while (positionNode != nullptr) {
        auto position = deserializePosition(*positionNode);
        if (position == nullptr) {
            return {};              // Stops as soon as an error is detected.
        }
        positions.emplace_back(*position);

        // Next sibling.
        positionNode = positionNode->getNextElementWithTagName(nodePositionRoot);
    }

    return positions;
}

}   // namespace arkostracker
