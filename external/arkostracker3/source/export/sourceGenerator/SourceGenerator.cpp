#include "SourceGenerator.h"

#include "../../utils/StringUtil.h"
#include "SourceGeneratorException.h"

namespace arkostracker 
{

SourceGenerator::SourceGenerator(SourceGeneratorConfiguration pConfiguration, juce::OutputStream& pOutputStream) noexcept :
        configuration(std::move(pConfiguration)),
        outputStream(pOutputStream),
        disarkSourceGenerator(*this),
        isEndOfFile(false)
{
}

SourceGenerator& SourceGenerator::addEmptyLine()
{
    writeInOutputStream(juce::String(), false, true);

    return *this;
}

SourceGenerator& SourceGenerator::declareAddressChange(const int address)
{
    if (address < 0) {
        throw SourceGeneratorException("Address is negative: " + juce::String(address));
    }

    const auto addressString = juce::String(address);
    const auto line = buildFromPlaceholder(configuration.getAddressChangeDeclaration(), addressString);
    writeInOutputStream(line, true, true);

    return *this;
}

SourceGenerator& SourceGenerator::declareByte(const int value, const juce::String& comment)
{
    if ((value < -128) || (value > 255)) {
        throw SourceGeneratorException("Invalid byte declared: " + juce::String(value));
    }
    declareByte(juce::String(value), comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareByte(const juce::String& expression, const juce::String& comment)
{
    const auto line = buildFromPlaceholder(configuration.getByteDeclaration(), expression);
    writeInOutputStream(line, true, comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareWord(const int value, const juce::String& comment)
{
    if ((value < -32768) || (value > 65535)) {
        throw SourceGeneratorException("Invalid word declared: " + juce::String(value));
    }
    declareWord(juce::String(value), comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareWord(const juce::String& expression, const juce::String& comment)
{
    const auto line = buildFromPlaceholder(configuration.getWordDeclaration(), expression);
    writeInOutputStream(line, true, comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareString(const juce::String& string)
{
    const auto line = buildFromPlaceholder(configuration.getStringDeclaration(), string);
    writeInOutputStream(line, true, true);

    return *this;
}

SourceGenerator& SourceGenerator::declareZeroTerminatedString(const juce::String& string, const juce::String& comment)
{
    // Encodes the String, with quotes, if not empty.
    if (!string.isEmpty()) {
        declareString(string);
    }
    // Adds the 0 at the end.
    declareByte(0, comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareAddress(const int address, const juce::String& comment)
{
    if (address < 0) {
        throw SourceGeneratorException("Invalid address declared: " + juce::String(address));
    }
    declareAddress(juce::String(address), comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareAddress(const juce::String& expression, const juce::String& comment)
{
    const auto line = buildFromPlaceholder(configuration.getAddressDeclaration(), expression);
    writeInOutputStream(line, true, comment);

    return *this;
}

SourceGenerator& SourceGenerator::declareComment(const juce::String& comment)
{
    if (configuration.doesGenerateComments()) {
        writeInOutputStream(juce::String(), false, comment);
    }

    return *this;
}

SourceGenerator& SourceGenerator::declareFourBytes(const int fullValue, const juce::String& comment)
{
    if (isEndOfFile) {
        throw SourceGeneratorException("The end of file has already been declared.");
    }

    // Splits the value into 4 bytes.
    const auto byte0 = (fullValue >> 0U) & 0xff;
    const auto byte1 = (fullValue >> 8) & 0xff;
    const auto byte2 = (fullValue >> 16) & 0xff;
    const auto byte3 = (fullValue >> 24) & 0xff;
    if (configuration.isLittleEndian()) {
        declareByte(byte0, comment).declareByte(byte1).declareByte(byte2).declareByte(byte3);
    } else {
        declareByte(byte3, comment).declareByte(byte2).declareByte(byte1).declareByte(byte0);
    }

    return *this;
}

juce::String SourceGenerator::buildRelativeExpression(const juce::String& label, const juce::String& relativeToLabel) noexcept
{
    if (relativeToLabel.trim().isEmpty()) {
        return label;
    }
    return label + " - " + relativeToLabel;
}

SourceGenerator& SourceGenerator::declareAddressOrRelativeAddress(const juce::String& expression, const juce::String& relativeToExpression)
{
    if (relativeToExpression.trim().isEmpty()) {
        return declareAddress(expression);
    }

    return declareAddress(buildRelativeExpression(expression, relativeToExpression));
}

SourceGenerator& SourceGenerator::declareConstant(const juce::String& constant, const juce::String& value, const juce::String& comment)
{
    if (isEndOfFile) {
        throw SourceGeneratorException("The end of file has already been declared.");
    }

    // Writes "MY_LABEL = value" for example.
    const auto text = constant + " " + buildFromPlaceholder(configuration.getConstantDeclaration(), value);
    writeInOutputStream(text, false, comment, true);

    return *this;
}

void SourceGenerator::declareEndOfFile()
{
    writeInOutputStream(juce::String(), false, false);

    outputStream.flush();

    isEndOfFile = true;
}

SourceGenerator& SourceGenerator::declareLabel(const juce::String& label, const bool addReturnCarriage, const juce::String& comment)
{
    const auto line = buildFromPlaceholder(configuration.getLabelDeclaration(), label);
    writeInOutputStream(line, false, comment, addReturnCarriage);

    return *this;
}


// ======================================================
// Disark.

SourceGenerator& SourceGenerator::declareByteRegionStart()
{
    disarkSourceGenerator.declareByteRegionStart();
    return *this;
}

SourceGenerator& SourceGenerator::declareByteRegionEnd()
{
    disarkSourceGenerator.declareByteRegionEnd();
    return *this;
}

SourceGenerator& SourceGenerator::declarePointerRegionStart()
{
    disarkSourceGenerator.declarePointerRegionStart();
    return *this;
}

SourceGenerator& SourceGenerator::declarePointerRegionEnd()
{
    disarkSourceGenerator.declarePointerRegionEnd();
    return *this;
}

SourceGenerator& SourceGenerator::declareWordForceNonReference(const int value, const juce::String& comment)
{
    disarkSourceGenerator.declareWordForceNonReference(value, comment);
    return *this;
}

SourceGenerator& SourceGenerator::declareWordForceReference(const juce::String& expression, const juce::String& comment)
{
    disarkSourceGenerator.declareWordForceReference(expression, comment);
    return *this;
}

SourceGenerator& SourceGenerator::declareForceNonReferenceAreaDuring(const int durationInBytes)
{
    disarkSourceGenerator.declareForceNonReferenceAreaDuring(durationInBytes);
    return *this;
}

void SourceGenerator::setPrefixForDisark(juce::String prefix) noexcept
{
    disarkSourceGenerator.setPrefix(std::move(prefix));
}

SourceGenerator& SourceGenerator::declareExternalLabel(const juce::String& label)
{
    disarkSourceGenerator.declareExternalLabel(label);
    return *this;
}


// ========================================================================

void SourceGenerator::writeInOutputStream(const juce::String& string, const bool addTabulation, const bool addReturnCarriage) const
{
    writeInOutputStream(string, addTabulation, juce::String(), addReturnCarriage);
}

void SourceGenerator::writeInOutputStream(const juce::String& string, const bool addTabulation, const juce::String& comment, const bool addReturnCarriage) const
{
    if (isEndOfFile) {
        throw SourceGeneratorException("The end of file has already been declared.");
    }

    // Possible tabulation, then the text, the possible comment, then the possible return carriage (always if a comment is present).

    if (string.indexOf("\n") >= 0) {
        // Multiple lines in the mnemonic.
        const auto strings = StringUtil::split(string, "\n");
        auto firstLine = true;
        for (const auto& line : strings) {
            writeStringInOutputStream(line.removeCharacters("\r"), addTabulation);      // Remove '\r' in case a \r\n was given
            // Writes the possible comment, but only if on the first line!
            if (firstLine) {
                writeCommentIfAny(comment);
                firstLine = false;
            }
            writeReturnCarriage();
        }
    } else {
        // Single line in the mnemonic.
        writeStringInOutputStream(string, addTabulation);
        const auto addSeparatorBeforeComment = string.isNotEmpty();
        writeCommentIfAny(comment, addSeparatorBeforeComment);

        if (addReturnCarriage || comment.isNotEmpty()) {          // Always force the RC if a comment is present.
            writeReturnCarriage();
        }
    }
}

juce::String SourceGenerator::buildFromPlaceholder(const juce::String& expression, const juce::String& argument) noexcept
{
    return expression.replace(SourceGeneratorConfiguration::placeholder, argument);
}

void SourceGenerator::writeStringInOutputStream(const juce::String& string, const bool addTabulation) const
{
    if (addTabulation) {
        outputStream.writeText(configuration.getTabulation(), false, false, nullptr);
    }
    outputStream.writeText(string, false, false, nullptr);
}

void SourceGenerator::writeReturnCarriage() const
{
    outputStream.writeText(outputStream.getNewLineString(), false, false, nullptr);
}

void SourceGenerator::writeCommentIfAny(const juce::String& comment, const bool addSpacesBeforeComment) const
{
    if (comment.isEmpty() || !configuration.doesGenerateComments()) {
        return;
    }

    const auto commentText = buildFromPlaceholder(configuration.getCommentDeclaration(), comment);
    const auto separator = addSpacesBeforeComment ? "    " : juce::String();
    outputStream.writeText(separator + commentText, false, false, nullptr);
}


}   // namespace arkostracker

