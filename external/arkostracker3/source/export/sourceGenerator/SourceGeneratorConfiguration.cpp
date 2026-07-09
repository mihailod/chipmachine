#include "SourceGeneratorConfiguration.h"

namespace arkostracker 
{

const juce::String SourceGeneratorConfiguration::placeholder = "{x}";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String SourceGeneratorConfiguration::defaultConstantPlaceholder = "equ {x}";       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

SourceGeneratorConfiguration::SourceGeneratorConfiguration(juce::String pAddressChangeDeclaration, juce::String pCommentDeclaration, bool pGenerateComments,
                                                           juce::String pByteDeclaration, juce::String pWordDeclaration, juce::String pAddressDeclaration,
                                                           juce::String pStringDeclaration, juce::String pConstantDeclaration, juce::String pLabelDeclaration,
                                                           const bool pLittleEndian, juce::String pSourceFileExtension,
                                                           juce::String pBinaryFileExtension, const int pTabulationLength) noexcept :
        addressChangeDeclaration(std::move(pAddressChangeDeclaration)),
        commentDeclaration(std::move(pCommentDeclaration)),
        generateComments(pGenerateComments),
        byteDeclaration(std::move(pByteDeclaration)),
        wordDeclaration(std::move(pWordDeclaration)),
        addressDeclaration(std::move(pAddressDeclaration)),
        stringDeclaration(std::move(pStringDeclaration)),
        constantDeclaration(std::move(pConstantDeclaration)),
        labelDeclaration(std::move(pLabelDeclaration)),
        littleEndian(pLittleEndian),
        sourceFileExtension(std::move(pSourceFileExtension)),
        binaryFileExtension(std::move(pBinaryFileExtension)),
        tabulation()
{
    assertPlaceHolder(addressChangeDeclaration);
    assertPlaceHolder(byteDeclaration);
    assertPlaceHolder(wordDeclaration);
    assertPlaceHolder(addressDeclaration);
    assertPlaceHolder(stringDeclaration);
    assertPlaceHolder(labelDeclaration);
    assertPlaceHolder(constantDeclaration);
    assertPlaceHolder(commentDeclaration);

    // Generates the tabulation String once and for all. Not the greatest of code...
    jassert(pTabulationLength > 0);
    for (auto index = 0; index < pTabulationLength; ++index) {
        tabulation += " ";
    }
}

void SourceGeneratorConfiguration::assertPlaceHolder(const juce::String& string) noexcept
{
    jassert(string.contains(placeholder)); (void)string;
}

SourceGeneratorConfiguration SourceGeneratorConfiguration::buildZ80(bool generateComments) noexcept
{
    return { "org {x}", "; {x}", generateComments, "db {x}", "dw {x}", "dw {x}",
             "db \"{x}\"",
        // Better NOT to use EQU, else the multiple song/SFX player config cannot be replaced by new values.
        // If a problem, need to add a new "variable" method instead of "constant".
        "= {x}",
        "{x}", true, "asm", "bin", 4 };
}

SourceGeneratorConfiguration SourceGeneratorConfiguration::build68000(bool generateComments) noexcept
{
    return { "org {x}", "; {x}", generateComments, "dc.b {x}", "dc.w {x}",
             "dc.w {x}",
             "dc.b \"{x}\"", "equ {x}", "{x}", false, "s", "bin", 4 };
}

SourceGeneratorConfiguration SourceGeneratorConfiguration::build6502Acme() noexcept
{
    return { "*={x}", "; {x}", true, "!byte {x}", "!word {x}",
             "!word {x}",
             "!text \"{x}\"", "= {x}", "{x}", true, "a", "b", 4 };
}

SourceGeneratorConfiguration SourceGeneratorConfiguration::build6502Mads() noexcept
{
    return { "ORG {x}", "; {x}", true, ".byte {x}", ".word {x}",
             ".word {x}",
             ".text \"{x}\"", "equ {x}", "{x}", true, "ASM", "obx", 4 };
}

const juce::String& SourceGeneratorConfiguration::getTabulation() const noexcept
{
    return tabulation;
}

int SourceGeneratorConfiguration::getTabulationLength() const noexcept
{
    return tabulation.length();
}

const juce::String& SourceGeneratorConfiguration::getCommentDeclaration() const noexcept
{
    return commentDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getByteDeclaration() const noexcept
{
    return byteDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getWordDeclaration() const noexcept
{
    return wordDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getAddressDeclaration() const noexcept
{
    return addressDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getAddressChangeDeclaration() const noexcept
{
    return addressChangeDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getLabelDeclaration() const noexcept
{
    return labelDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getStringDeclaration() const noexcept
{
    return stringDeclaration;
}

const juce::String& SourceGeneratorConfiguration::getConstantDeclaration() const noexcept
{
    return constantDeclaration;
}

bool SourceGeneratorConfiguration::doesGenerateComments() const noexcept
{
    return generateComments;
}

bool SourceGeneratorConfiguration::isLittleEndian() const noexcept
{
    return littleEndian;
}

const juce::String& SourceGeneratorConfiguration::getSourceFileExtension() const noexcept
{
    return sourceFileExtension;
}

const juce::String& SourceGeneratorConfiguration::getBinaryFileExtension() const noexcept
{
    return binaryFileExtension;
}

}   // namespace arkostracker
