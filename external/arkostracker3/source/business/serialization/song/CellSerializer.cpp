#include "CellSerializer.h"

#include "../../../song/cells/Cell.h"
#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String CellSerializer::nodeCell = "cell";                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeIndex = "index";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeNote = "note";                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeInstrument = "instrument";           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeEffect = "effect";                   // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeEffectName = "name";                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CellSerializer::nodeEffectLogicalValue = "logicalValue"; // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> CellSerializer::serialize(const int cellIndex, const Cell& cell) noexcept // NOLINT(*-convert-member-functions-to-static)
{
    auto cellRoot = std::make_unique<juce::XmlElement>(nodeCell);

    XmlHelper::addValueNode(*cellRoot, nodeIndex, cellIndex);

    // Note and instrument, if present.
    const auto noteOptional = cell.getNote();
    if (noteOptional.isPresent()) {
        XmlHelper::addValueNode(*cellRoot, nodeNote, noteOptional.getValue().getNote());

        // Encodes the instrument only if the note is present.
        const auto instrumentOptional = cell.getInstrument();
        if (instrumentOptional.isPresent()) {
            XmlHelper::addValueNode(*cellRoot, nodeInstrument, instrumentOptional.getValue());
        }
    }

    // Effects, if any.
    for (const auto& effectAndValue : cell.getEffects().getExistingEffects()) {
        auto& effectNode = XmlHelper::addNode(*cellRoot, nodeEffect);
        const auto effectIndex = effectAndValue.first;
        const auto effect = effectAndValue.second;

        const auto effectString = EffectUtil::toSerializedString(effect.getEffect());

        XmlHelper::addValueNode(effectNode, nodeIndex, effectIndex);
        XmlHelper::addValueNode(effectNode, nodeEffectName, effectString);
        XmlHelper::addValueNode(effectNode, nodeEffectLogicalValue, effect.getEffectLogicalValue());
    }

    return cellRoot;
}

std::pair<Cell, int> CellSerializer::deserialize(const juce::XmlElement& cellNode) noexcept // NOLINT(*-convert-member-functions-to-static)
{
    const auto index = XmlHelper::readInt(cellNode, nodeIndex, -1);
    const auto rawNote = XmlHelper::readInt(cellNode, nodeNote, -1);
    const OptionalValue<Note> note = (rawNote >= 0) ? Note::buildNote(rawNote) : OptionalValue<Note>();
    const auto rawInstrument = XmlHelper::readInt(cellNode, nodeInstrument, -1);
    const OptionalInt instrument = (rawInstrument >= 0) ? rawInstrument : OptionalInt();
    Cell cell(note, instrument);

    // Effects?
    for (const auto* effectNode : XmlHelper::getChildrenList(cellNode, nodeEffect)) {
        auto effectIndex = XmlHelper::readInt(*effectNode, nodeIndex, -1);
        if (effectIndex < 0) {
            jassertfalse;
            effectIndex = 0;
        }
        const auto effectString = XmlHelper::readString(*effectNode, nodeEffectName);
        const auto effectOptional = EffectUtil::fromSerializedString(effectString);
        if (effectOptional.isAbsent()) {
            jassertfalse;           // Unable to parse the effect name.
            continue;
        }
        const auto effectLogicalValue = XmlHelper::readInt(*effectNode, nodeEffectLogicalValue);

        // Creates the CellEffect and adds it.
        const auto cellEffect = CellEffect::buildFromLogicalValue(effectOptional.getValue(), effectLogicalValue);
        cell.setEffect(effectIndex, cellEffect);
    }

    return { cell, index };
}

}   // namespace arkostracker
