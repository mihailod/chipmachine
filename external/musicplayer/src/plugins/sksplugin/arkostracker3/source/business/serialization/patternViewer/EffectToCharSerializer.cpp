#include "EffectToCharSerializer.h"

#include "../../patternViewer/DisplayedEffects.h"
#include "../../../utils/XmlHelper.h"

namespace arkostracker 
{

const juce::String EffectToCharSerializer::nodeRoot = "effectsToChar";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String EffectToCharSerializer::nodeEntry = "effectToChar";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String EffectToCharSerializer::nodeEffect = "effect";                       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String EffectToCharSerializer::nodeChar = "char";                           // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<juce::XmlElement> EffectToCharSerializer::serialize(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) noexcept
{
    auto root = std::make_unique<juce::XmlElement>(nodeRoot);

    // Creates one entry per effect.
    for (const auto& entry : effectToChar) {
        const auto effectString = EffectUtil::toSerializedString(entry.first);
        const auto charString = juce::String::charToString(entry.second);

        auto& entryNode = XmlHelper::addNode(*root, nodeEntry);
        XmlHelper::addValueNode(entryNode, nodeEffect, effectString);
        XmlHelper::addValueNode(entryNode, nodeChar, charString);
    }

    return root;
}

std::unordered_map<Effect, juce::juce_wchar> EffectToCharSerializer::deserialize(const juce::XmlElement& rootNode) noexcept
{
    // Uses the default one, it will be replaced by new items. If some are missing, the default ones will still be present.
    auto effectToChar = DisplayedEffects::getDefaultEffectToDisplayedChar();

    for (const auto* entry : XmlHelper::getChildrenList(rootNode, nodeEntry)) {
        const auto effectString = XmlHelper::readString(*entry, nodeEffect);
        const auto charString = XmlHelper::readString(*entry, nodeChar);

        const auto effectOptional = EffectUtil::fromSerializedString(effectString);
        if (effectOptional.isAbsent()) {
            jassertfalse;           // Shouldn't happen.
            continue;
        }
        if (charString.length() != 1) {
            jassertfalse;           // Should be one char only.
            continue;
        }
        // Replaces the Effect value from the XmlHelper::
        const auto c = charString[0];
        effectToChar[effectOptional.getValue()] = c;
    }

    return effectToChar;
}


}   // namespace arkostracker

