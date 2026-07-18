#include "TextEditorUtil.h"

namespace arkostracker 
{

const juce::String TextEditorUtil::restrictionInt("0123456789");                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionPositiveFloat("0123456789.");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionDecimal("-0123456789.");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionSource("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionHexadecimal("0123456789abcdefABCDEF");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionQuickEditPositive("ABCDEFabcdef0123456789 ,");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionQuickEditRelative("-ABCDEFabcdef0123456789 ,");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String TextEditorUtil::restrictionFilename("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789+-");             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

juce::TextEditor::LengthAndCharacterRestriction TextEditorUtil::buildRestrictionForInt(const int digitCount) noexcept
{
    return { digitCount, restrictionInt };
}

juce::TextEditor::LengthAndCharacterRestriction TextEditorUtil::buildRestrictionForPositiveFloat(const int digitCount) noexcept
{
    return { digitCount, restrictionPositiveFloat };
}

juce::TextEditor::LengthAndCharacterRestriction TextEditorUtil::buildRestrictionForDecimal(const int digitCount) noexcept
{
    return { digitCount, restrictionDecimal };
}

juce::TextEditor::LengthAndCharacterRestriction TextEditorUtil::buildRestrictionForSource(const int digitCount) noexcept
{
    return { digitCount, restrictionSource };
}

juce::TextEditor::LengthAndCharacterRestriction TextEditorUtil::buildRestrictionForHexadecimal(const int digitCount) noexcept
{
    return { digitCount, restrictionHexadecimal };
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> TextEditorUtil::buildRestrictionForQuickEditPositive(const int digitCount) noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(digitCount, restrictionQuickEditPositive);
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> TextEditorUtil::buildRestrictionForQuickEditRelative(const int digitCount) noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(digitCount, restrictionQuickEditRelative);
}

std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> TextEditorUtil::buildRestrictionForFilename(const int digitCount) noexcept
{
    return std::make_unique<juce::TextEditor::LengthAndCharacterRestriction>(digitCount, restrictionFilename);
}

}   // namespace arkostracker
