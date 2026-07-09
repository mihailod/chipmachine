#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Utility class for Text Editors. */
class TextEditorUtil
{
public:
    static const juce::String restrictionSource;         // Restricts to the allowed characters in the ASM editor.
    static const juce::String restrictionInt;            // Restricts to int digits.

    /** Prevents instantiation. */
    TextEditorUtil() = delete;

    /**
     * Builds a restriction for a TextEditor, to integer digits.
     * Use TextEditor::setInputFilter to use it (warning, the restriction must be kept in memory).
     * @param digitCount how many digits, may be <0.
     */
    static juce::TextEditor::LengthAndCharacterRestriction buildRestrictionForInt(int digitCount) noexcept;

    /**
     * Builds a restriction for a TextEditor, to integer digits, plus a comma.
     * Use TextEditor::setInputFilter to use it (warning, the restriction must be kept in memory).
     * @param digitCount how many digits, may be <0.
     */
    static juce::TextEditor::LengthAndCharacterRestriction buildRestrictionForPositiveFloat(int digitCount) noexcept;

    /**
     * Builds a restriction for a TextEditor, to decimal digits.
     * Use TextEditor::setInputFilter to use it (warning, the restriction must be kept in memory).
     * @param digitCount how many digits, may be <0.
     */
    static juce::TextEditor::LengthAndCharacterRestriction buildRestrictionForDecimal(int digitCount) noexcept;

    /**
     * @return a Restriction for a TextEditor, for source (letters, underscore, digits).
     * @param digitCount how many digits, may be <0.
     */
    static juce::TextEditor::LengthAndCharacterRestriction buildRestrictionForSource(int digitCount = 16) noexcept;

    /**
     * @return a Restriction for a TextEditor, for hexadecimal.
     * @param digitCount how many digits, may be <0.
     */
    static juce::TextEditor::LengthAndCharacterRestriction buildRestrictionForHexadecimal(int digitCount = 4) noexcept;

    /**
     * @return a Restriction for a TextEditor, for quick edit (hex, comma, space).
     * @param digitCount how many digits, may be <0.
     */
    static std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> buildRestrictionForQuickEditPositive(int digitCount) noexcept;

    /**
     * @return a Restriction for a TextEditor, for quick edit (hex, minus, comma, space).
     * @param digitCount how many digits, may be <0.
     */
    static std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> buildRestrictionForQuickEditRelative(int digitCount) noexcept;

    /**
     * @return a Restriction for a file name (letters, numbers, a few special chars).
     * @param digitCount how many digits, may be <0.
     */
    static std::unique_ptr<juce::TextEditor::LengthAndCharacterRestriction> buildRestrictionForFilename(int digitCount) noexcept;

private:
    static const juce::String restrictionPositiveFloat;          // Restricts to digits and a comma.
    static const juce::String restrictionDecimal;          // Restricts to decimal and a comma.
    static const juce::String restrictionHexadecimal;    // Restricts to hexadecimal.

    static const juce::String restrictionQuickEditPositive;
    static const juce::String restrictionQuickEditRelative;

    static const juce::String restrictionFilename;
};

}   // namespace arkostracker
