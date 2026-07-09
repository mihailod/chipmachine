#include "DisarkSourceGenerator.h"

#include <utility>

#include "SourceGenerator.h"
#include "SourceGeneratorException.h"
#include "../../disark/base/AreaTag.h"

namespace arkostracker 
{

DisarkSourceGenerator::DisarkSourceGenerator(SourceGenerator& pSourceGenerator) noexcept :
    sourceGenerator(pSourceGenerator),
    nextId(-1),
    prefix()
{
}

void DisarkSourceGenerator::setPrefix(juce::String newPrefix) noexcept
{
    prefix = std::move(newPrefix);
}

void DisarkSourceGenerator::declareByteRegionStart()
{
    openRegion(AreaTag::tagByteRegionStart);
}
void DisarkSourceGenerator::declareByteRegionEnd()
{
    closeRegion(AreaTag::tagByteRegionEnd);
}

void DisarkSourceGenerator::declareWordRegionStart()
{
    openRegion(AreaTag::tagWordRegionStart);
}
void DisarkSourceGenerator::declareWordRegionEnd()
{
    closeRegion(AreaTag::tagWordRegionEnd);
}

void DisarkSourceGenerator::declareWordForceReference(const juce::String& expression, const juce::String& comment)
{
    declareLabelForWordForceReference();
    sourceGenerator.declareWord(expression, comment);
}
void DisarkSourceGenerator::declareWordForceReference(int value)
{
    declareLabelForWordForceReference();
    sourceGenerator.declareWord(value);
}
void DisarkSourceGenerator::declareWordForceNonReference(const juce::String& expression)
{
    declareLabelForWordForceNonReference();
    sourceGenerator.declareWord(expression);
}
void DisarkSourceGenerator::declareWordForceNonReference(int value, const juce::String& comment)
{
    declareLabelForWordForceNonReference();
    sourceGenerator.declareWord(value, comment);
}

void DisarkSourceGenerator::declareLabelForWordForceReference()
{
    insertInlineRegion(AreaTag::tagForceOneWordAreaWithForceReference);
}
void DisarkSourceGenerator::declareLabelForWordForceNonReference()
{
    insertInlineRegion(AreaTag::tagForceOneWordAreaWithForceNonReference);
}

void DisarkSourceGenerator::declarePointerRegionStart()
{
    openRegion(AreaTag::tagPointerRegionStart);
}
void DisarkSourceGenerator::declarePointerRegionEnd()
{
    closeRegion(AreaTag::tagPointerRegionEnd);
}

void DisarkSourceGenerator::declareForceNonReferenceAreaStart()
{
    openRegion(AreaTag::tagForceNonReferenceAreaStart);
}
void DisarkSourceGenerator::declareForceNonReferenceAreaEnd()
{
    closeRegion(AreaTag::tagForceNonReferenceAreaEnd);
}

void DisarkSourceGenerator::declareForceNonReferenceAreaDuring(int durationInBytes)
{
    jassert((durationInBytes >= 1) && (durationInBytes <= 9));
    insertInlineRegion(AreaTag::baseTagForceNonReferenceDuring + juce::String(durationInBytes));
}

void DisarkSourceGenerator::declareForceReferenceAreaStart()
{
    openRegion(AreaTag::tagForceReferenceAreaStart);
}

void DisarkSourceGenerator::declareForceReferenceAreaEnd()
{
    closeRegion(AreaTag::tagForceReferenceAreaEnd);
}

void DisarkSourceGenerator::declareExternalLabel(const juce::String& label)
{
    sourceGenerator.declareLabel(label + AreaTag::tagGenerateExternalLabel);
}

// ======================================================

void DisarkSourceGenerator::openRegion(const juce::String& tag)
{
    ++nextId;
    openedStructureId.emplace_back(nextId);     // Stores the new ID, as we have opened the structure.

    encodeAreaLabelAndEndOfLine(tag, nextId);
}

void DisarkSourceGenerator::closeRegion(const juce::String& tag)
{
    // Gets the previous ID. If there is nothing, critical error!
    if (openedStructureId.empty()) {
        throw SourceGeneratorException("All structure were closed, yet trying to close another one.");
    }

    // Gets the latest element, removing it from the stack.
    int id = openedStructureId.back();
    openedStructureId.pop_back();

    encodeAreaLabelAndEndOfLine(tag, id);
}

void DisarkSourceGenerator::insertInlineRegion(const juce::String& tag)
{
    ++nextId;
    encodeAreaLabelAndEndOfLine(tag, nextId);
}

void DisarkSourceGenerator::encodeAreaLabelAndEndOfLine(const juce::String& tagLabel, int id)
{
    sourceGenerator.declareLabel(prefix + tagLabel + juce::String(id));
}

}   // namespace arkostracker

