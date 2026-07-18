#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SpecialCell;

/** Serializes/deserializes Special Cells. */
class SpecialCellSerializer
{
public:
    // Constants are made public for testing.
    static const juce::String nodeSpecialCell;
    static const juce::String nodeIndex;
    static const juce::String nodeValue;

    /**
     * Serializes a Cell into XML.
     * @param cellIndex the index of the Cell.
     * @param cell the Cell.
     * @return the node. May be empty, but not the pointer.
     */
    std::unique_ptr<juce::XmlElement> serialize(int cellIndex, const SpecialCell& specialCell) noexcept;

    /**
     * Deserializes a SpecialCell from XML.
     * @param specialCellNode the SpecialCell node ("cell").
     * @return the SpecialCell, and its encoded index (-1 if not found).
     */
    std::pair<SpecialCell, int> deserialize(const juce::XmlElement& specialCellNode) noexcept;
};


}   // namespace arkostracker

