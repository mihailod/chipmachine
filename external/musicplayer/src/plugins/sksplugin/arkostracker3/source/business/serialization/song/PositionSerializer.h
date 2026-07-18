#pragma once

#include <memory>
#include <vector>

#include "../../../song/subsong/Position.h"

namespace arkostracker 
{

/** Serializes/deserializes the Position. */                // TODO TU this.
class PositionSerializer
{
public:
    /** Prevents instantiation. */
    PositionSerializer() = delete;

    /**
     * Serializes to XML the given Position.
     * @param position the object to serialize.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const Position& position) noexcept;

    /**
     * Serializes to XML the given Positions.
     * @param positions the objects to serialize.
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::vector<Position>& positions) noexcept;

    /**
     * Deserializes an XML into one Position.
     * @param positionRootElement the "position" root XML Element.
     * @return the Position, or null if an error occurred.
     */
    static std::unique_ptr<Position> deserializePosition(const juce::XmlElement& positionRootElement) noexcept;

    /**
     * Deserializes an XML into Positions.
     * @param positionsRoot the XML Element which contains all the Position nodes.
     * @return the Positions, or empty if an error occurred.
     */
    static std::vector<Position> deserializePositions(const juce::XmlElement& positionsRoot) noexcept;

private:
    static const juce::String nodePositionsRoot;

    static const juce::String nodePositionRoot;

    static const juce::String nodePatternIndex;
    static const juce::String nodeHeight;
    static const juce::String nodeMarkerName;
    static const juce::String nodeMarkerColor;

    static const juce::String nodeTranspositions;
    static const juce::String nodeTransposition;
    static const juce::String nodeChannelWithTransposition;
    static const juce::String nodeTranspositionValue;
};

}   // namespace arkostracker
