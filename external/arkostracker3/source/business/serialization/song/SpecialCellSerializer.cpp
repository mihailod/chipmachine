#include "SpecialCellSerializer.h"

#include "../../../song/cells/SpecialCell.h"
#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String SpecialCellSerializer::nodeSpecialCell = "cell";                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SpecialCellSerializer::nodeIndex = "index";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SpecialCellSerializer::nodeValue = "value";                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> SpecialCellSerializer::serialize(int cellIndex, const SpecialCell& specialCell) noexcept
{
    auto specialCellRoot = std::make_unique<juce::XmlElement>(nodeSpecialCell);

    XmlHelper::addValueNode(*specialCellRoot, nodeIndex, cellIndex);

    // Any value, if present.
    if (!specialCell.isEmpty()) {
        XmlHelper::addValueNode(*specialCellRoot, nodeValue, specialCell.getValue());
    }

    return specialCellRoot;
}

std::pair<SpecialCell, int> SpecialCellSerializer::deserialize(const juce::XmlElement& specialCellNode) noexcept
{
    const auto index = XmlHelper::readInt(specialCellNode, nodeIndex, -1);
    const auto value = XmlHelper::readInt(specialCellNode, nodeValue);

    return { SpecialCell::buildSpecialCell(value), index };
}


}   // namespace arkostracker

