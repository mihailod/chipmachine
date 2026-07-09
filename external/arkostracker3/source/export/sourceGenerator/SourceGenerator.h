#pragma once

#include <juce_core/juce_core.h>

#include "DisarkSourceGenerator.h"
#include "SourceGeneratorConfiguration.h"

namespace arkostracker 
{

/**
 * Class to generate a source file. Configuration defines the global syntax of the mnemonics, so that Z80 or 68000 instructions can be written.
 * Contrary to AT2, each type of expression are using placeholders. Example: dw {X}. They are possibly on several lines.
 *
 * If using Disark, you should call "setPrefixForDisark" after instantiation.
 */
class SourceGenerator
{
public:
    /**
     * Constructor.
     * @param configuration the configuration to indicate the format of the generated source.
     * @param outputStream the stream where to write the source.
     */
    SourceGenerator(SourceGeneratorConfiguration configuration, juce::OutputStream& outputStream) noexcept;

    /**
     * Adds an empty line.
     * @return this object to allow chaining.
     */
    SourceGenerator& addEmptyLine();

    /**
     * Adds an address change (ORG or equivalent).
     * @param address the address. Must be positive.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareAddressChange(int address);

    /**
     * Adds a label. Automatically declares a new line, as a label must be at the start of the line.
     * Does NOT end the line.
     * @param label the label ("Mylabel"), without any prefix or suffix.
     * @param addReturnCarriage true to add a return carriage. Forced to true if a comment is present.
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareLabel(const juce::String& label, bool addReturnCarriage = true, const juce::String& comment = juce::String());

    /**
     * Declares a byte, and adds a new line. The expression is stored as-is.
     * @param expression the expression ("12", "0x1f", "label + 2 * 5", etc.).
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareByte(const juce::String& expression, const juce::String& comment= juce::String());

    /**
     * Declares a byte, as an integer, and adds a new line. The expression is stored as-is.
     * @param value the value. May be positive or negative, between -128 and 255.
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareByte(int value, const juce::String& comment = juce::String());

    /**
     * Declares a word (16 bits), and adds a new line.
     * @param value the value. May be positive or negative, between -32768 and 65535.
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareWord(int value, const juce::String& comment = juce::String());

    /**
     * Declares a word (16 bits), and adds a new line. The expression is stored as-is.
     * @param expression the expression ("12", "0x1f", "label + 2 * 5", etc.).
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareWord(const juce::String& expression, const juce::String& comment = juce::String());

    /**
     * Declares a String, and adds a new line. Quotes must not be added.
     * @param string the string, without any quotes (unless they are wanted!).
     * @return this object to allow chaining.
     */
    SourceGenerator& declareString(const juce::String& string);

    /**
     * Declares a String (as bytes), and adds a 0 at the end.
     * @param string the String to encode.
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareZeroTerminatedString(const juce::String& string, const juce::String& comment = juce::String());

    /**
     * Declares an address (16 bits), and adds a new line.
     * @param address the address. Must be positive.
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareAddress(int address, const juce::String& comment = juce::String());

    /**
     * Declares an address (16 bits), and adds a new line. The expression is stored as-is.
     * @param expression the expression ("12", "0x1f", "label + 2 * 5", etc.).
     * @param comment a possible comment, or empty if there is none.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareAddress(const juce::String& expression, const juce::String& comment = juce::String());

    /**
     * Declares a comment at the beginning of a line. A new line is automatically generated after it.
     * Nothing happens at all if the comments must not be generated.
     * @param comment the comment, without the comment block. If empty, the comment is not generated (but the new line is anyway).
     * @return this object to allow chaining.
     */
    SourceGenerator& declareComment(const juce::String& comment);

    /**
     * Declares 4 bytes from an integer. A return carriage is added between each number and at the end.
     * The endian-ness of the configuration is used to know how to encode them.
     * @param fullValue the value.
     * @param comment the comment, without the comment block. If empty, the comment is not generated (but the new line is anyway).
     * @return this object to allow chaining.
     */
    SourceGenerator& declareFourBytes(int fullValue, const juce::String& comment = juce::String());

    /**
     * Builds a String consisting of the given label, and the possible label it is relative to.
     * For example (<label> - <relativeToLabel>). If the latter is empty, only the label is returned.
     * @param label the label.
     * @param relativeToLabel the possible relative to label, or empty if there is none.
     * @return the built expression.
     */
    static juce::String buildRelativeExpression(const juce::String& label, const juce::String& relativeToLabel) noexcept;

    /**
     * Declares an address, possibly relative to another one if not empty. Both expressions are stored as-is.
     * This may look like "<dw mnemonic> <expression> - <relativeToExpression>".
     * @param expression the expression ("12", "0x1f", "label + 2 * 5", etc.).
     * @param relativeToExpression the second expression. If empty, not used.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareAddressOrRelativeAddress(const juce::String& expression, const juce::String& relativeToExpression);

    /**
     * Defines a constant with a value.
     * @param constant the constant name.
     * @param value the value.
     * @param comment the comment, without the comment block. If empty, the comment is not generated (but the new line is anyway).
     * @return this object to allow chaining
     */
    SourceGenerator& declareConstant(const juce::String& constant, const juce::String& value, const juce::String& comment = juce::String());

    /** Declares the end of the file. A return carriage is added if the line was started. No more event must be sent after this. */
    void declareEndOfFile();


    // ======================================================
    // Disark.

    /** Declares a label for the start of a Byte region. */
    SourceGenerator& declareByteRegionStart();
    /** Declares a label for the end of a Byte region. */
    SourceGenerator& declareByteRegionEnd();

    /** Declares a label for the start of a Pointer region. */
    SourceGenerator& declarePointerRegionStart();
    /** Declares a label for the end of a Pointer region. */
    SourceGenerator& declarePointerRegionEnd();

    /**
     * Declares a label for a Word with a "force non-reference".
     * @param value the value.
     * @param comment a possible comment.
     */
    SourceGenerator& declareWordForceNonReference(int value, const juce::String& comment = juce::String());

    /**
     * Declares a label for a Word with a "force reference".
     * @param expression the expression to write.
     * @param comment a possible comment.
     */
    SourceGenerator& declareWordForceReference(const juce::String& expression, const juce::String& comment = juce::String());

    /** Declares a label for a "force non-reference" for X bytes (1-9). */
    SourceGenerator& declareForceNonReferenceAreaDuring(int durationInBytes);

    /**
     * Sets the prefix before each Disark label.
     * @param prefix the prefix.
     */
    void setPrefixForDisark(juce::String prefix) noexcept;

    /**
     * Adds an external Disark label. Automatically declares a new line.
     * @param label the label ("Mylabel"), without any prefix or suffix.
     * @return this object to allow chaining.
     */
    SourceGenerator& declareExternalLabel(const juce::String& label);

private:
    /**
     * Writes a String in the output stream.
     * @param string the String.
     * @param addTabulation true to add a tabulation first.
     * @param addReturnCarriage true to add a return carriage.
     */
    void writeInOutputStream(const juce::String& string, bool addTabulation = true, bool addReturnCarriage = true) const;

    /**
     * Writes a String in the output stream, with a possible comment.
     * @param string the String.
     * @param addTabulation true to add a tabulation first.
     * @param comment the comment (without ";").
     * @param addReturnCarriage true to add a return carriage. Ignored if there is a comment (true in this case).
     */
    void writeInOutputStream(const juce::String& string, bool addTabulation = true, const juce::String& comment = juce::String(), bool addReturnCarriage = true) const;

    /**
     * Writes a String in the output stream. It does NOT adds a return carriage.
     * @param string the String.
     * @param addTabulation true to add a tabulation first.
     */
    void writeStringInOutputStream(const juce::String& string, bool addTabulation) const;

    /** Writes a Return Carriage. */
    void writeReturnCarriage() const;
    /**
     * Writes the comment, if any. It does NOT write the return carriage.
     * Nothing happens at all if the comments must not be generated.
     * @param comment the comment, or empty.
     * @param addSpacesBeforeComment if true, a few spaces are added before the comment. Useful to separate mnemonic from the comments.
     */
    void writeCommentIfAny(const juce::String& comment, bool addSpacesBeforeComment = true) const;

    static juce::String buildFromPlaceholder(const juce::String& expression, const juce::String& argument) noexcept;

    SourceGeneratorConfiguration configuration;             // The configuration for the source.
    juce::OutputStream& outputStream;                       // The output stream where to generate the source.

    DisarkSourceGenerator disarkSourceGenerator;            // The Disark label generator.

    bool isEndOfFile;                                       // True if the end of file is declared.
};

}   // namespace arkostracker
