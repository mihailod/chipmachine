#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class Cell;

/** Serializes/deserializes a Cell. */
class CellSerializer
{
public:
    // Constants are made public for testing.
    static const juce::String nodeCell;
    static const juce::String nodeIndex;
    static const juce::String nodeNote;
    static const juce::String nodeInstrument;

    static const juce::String nodeEffect;
    static const juce::String nodeEffectName;
    static const juce::String nodeEffectLogicalValue;

    /**
     * Serializes a Cell into XML.
     * @param cellIndex the index of the Cell.
     * @param cell the Cell.
     * @return the node. May be empty, but not the pointer.
     */
    std::unique_ptr<juce::XmlElement> serialize(int cellIndex, const Cell& cell) noexcept;

    /**
     * Deserializes a Cell from XML.
     * @param cellNode the Cell node ("cell").
     * @return the cell, and its encoded index (-1 if not found).
     */
    std::pair<Cell, int> deserialize(const juce::XmlElement& cellNode) noexcept;
};


}   // namespace arkostracker

