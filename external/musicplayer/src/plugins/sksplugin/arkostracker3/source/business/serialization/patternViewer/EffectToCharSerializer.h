#pragma once

#include "unordered_map"
#include "../../../song/cells/Effect.h"

namespace arkostracker 
{

/** Class to serialize/deserialize the Effect to Char map. */
class EffectToCharSerializer
{
public:
    // Nodes public for testing.
    static const juce::String nodeRoot;
    static const juce::String nodeEntry;
    static const juce::String nodeEffect;
    static const juce::String nodeChar;

    /** Prevents instantiation. */
    EffectToCharSerializer() = delete;

    /**
     * Serializes to XML the given Effect To Char.
     * @param effectToChar the object to serialize. It must be valid!
     * @return the XML root node.
     */
    static std::unique_ptr<juce::XmlElement> serialize(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept;

    /**
     * Deserializes an XML into one Position.
     * @param rootNode the map root XML Element.
     * @return the map.
     */
    static std::unordered_map<Effect, juce::juce_wchar> deserialize(const juce::XmlElement& rootNode) noexcept;
};


}   // namespace arkostracker

