#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/**
 * Configuration for the SourceGenerator, allowing generating code for Z80, 68000, etc. but also for various assemblers.
 * Various constructor exist to easily build generic configuration (Z80, 68000, etc.).
 */
class SourceGeneratorConfiguration
{
public:
    static const juce::String placeholder;                      // Used to set variables in the mnemonic Strings.
    static const juce::String defaultConstantPlaceholder;

    /**
     * Constructor. Better use the static constructors, they are more handy in most cases.
     * @param addressChangeDeclaration the change address declaration ("org {x}" for example).
     * @param commentDeclaration the comment declaration ('; {x}' for example).
     * @param generateComments true if the comments must be generated.
     * @param byteDeclaration the byte declaration ("db {x}" for example).
     * @param wordDeclaration the word declaration ("dw {x}" for example).
     * @param addressDeclaration the address declaration ("dw {x}" for example).
     * @param stringDeclaration the string declaration ("ds {x}" for example).
     * @param constantDeclaration the constant declaration ("equ {x}" for example).
     * @param labelDeclaration the label declaration (".{x}:" for example).
     * @param littleEndian true if little endian.
     * @param sourceFileExtension the extension of the source file, without the dot.
     * @param binaryFileExtension the extension of the binary file, without the dot.
     * @param tabulationLength how many characters in the tabulation.
     */
    SourceGeneratorConfiguration(juce::String addressChangeDeclaration, juce::String commentDeclaration, bool generateComments,
                                 juce::String byteDeclaration, juce::String wordDeclaration, juce::String addressDeclaration, juce::String stringDeclaration,
                                 juce::String constantDeclaration,
                                 juce::String labelDeclaration, bool littleEndian,
                                 juce::String sourceFileExtension, juce::String binaryFileExtension, int tabulationLength) noexcept;

    /**
     * @return an instance for a Z80 configuration.
     * @param generateComments true to generate the comments.
    */
    static SourceGeneratorConfiguration buildZ80(bool generateComments = true) noexcept;

    /**
     * @return an instance for a 68000 configuration.
     * @param generateComments true to generate the comments.
     */
    static SourceGeneratorConfiguration build68000(bool generateComments = true) noexcept;

    /** @return an instance for a 6502 with ACME assembler configuration. */
    static SourceGeneratorConfiguration build6502Acme() noexcept;
    /** @return an instance for a 6502 with MADS assembler configuration. */
    static SourceGeneratorConfiguration build6502Mads() noexcept;

    /** @return the tabulation String. Do not serialize it, use the getTabulationLength instead. */
    const juce::String& getTabulation() const noexcept;
    /** @return the length of the tabulation. */
    int getTabulationLength() const noexcept;

    /** @return the comment declaration (with placeholder). */
    const juce::String& getCommentDeclaration() const noexcept;
    /** @return the byte declaration (with placeholder). */
    const juce::String& getByteDeclaration() const noexcept;
    /** @return the word declaration (with placeholder). */
    const juce::String& getWordDeclaration() const noexcept;
    /** @return the address declaration (with placeholder). */
    const juce::String& getAddressDeclaration() const noexcept;
    /** @return the byte address change declaration (with placeholder). */
    const juce::String& getAddressChangeDeclaration() const noexcept;
    /** @return the label declaration (with placeholder). */
    const juce::String& getLabelDeclaration() const noexcept;
    /** @return the String declaration (with placeholder). */
    const juce::String& getStringDeclaration() const noexcept;
    /** @return the constant declaration (with placeholder). */
    const juce::String& getConstantDeclaration() const noexcept;

    /** Returns true to generate comments, false to discard them all. */
    bool doesGenerateComments() const noexcept;
    /** Returns true if little endian. */
    bool isLittleEndian() const noexcept;

    /** Returns the extension for a source file, without the "." ("asm" for example). */
    const juce::String& getSourceFileExtension() const noexcept;
    /** Returns the extension for a binary file, without the "." ("bin" for example). */
    const juce::String& getBinaryFileExtension() const noexcept;

private:
    /** Makes sure the given String contains a placeholder. */
    static void assertPlaceHolder(const juce::String& string) noexcept;

    juce::String addressChangeDeclaration;          // The change address declaration ("org {x}" for example).
    juce::String commentDeclaration;                // The comment declaration ('; {x}' for example).
    bool generateComments;                          // True if the comments must be generated.
    juce::String byteDeclaration;                   // The byte declaration ("db {x}" for example).
    juce::String wordDeclaration;                   // The word declaration ("dw {x}" for example).
    juce::String addressDeclaration;                // The address declaration ("dw {x}" for example).
    juce::String stringDeclaration;                 // The string declaration ("ds {x}", quite often).
    juce::String constantDeclaration;               // The constant declaration ("equ {x}", quite often).
    juce::String labelDeclaration;                  // The label declaration (".{x}:" for example).
    bool littleEndian;                              // True if little endian.
    juce::String sourceFileExtension;               // The extension of the source file, without the dot.
    juce::String binaryFileExtension;               // The extension of the binary file, without the dot.
    juce::String tabulation;
};

}   // namespace arkostracker
