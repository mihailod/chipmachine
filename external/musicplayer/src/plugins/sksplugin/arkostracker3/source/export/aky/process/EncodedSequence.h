#pragma once

#include <vector>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

class SourceGenerator;

/**
 * A sequence represents a sequence of bytes, such as a declaration of a software period, a loop declaration, etc.
 * One or several sequences forms an EncodedLine.
 *
 * So there can be several sequence per line (very common)!
 *
 * A sequence can generate its source.
 */
class EncodedSequence
{
public:
    /** Static constructor for one byte and a possible comment. */
    static EncodedSequence buildOneByte(int byte1, const juce::String& comment);
    /** Static constructor for one word and a possible comment. */
    static EncodedSequence buildWord(int w, const juce::String& comment, bool littleEndian);
    /**
        Static constructor for one byte and a label, for the loop). Also, a possible comment.
        @param labelReference the label the Goto refers to.
        @param loopToIndex the index where to loop to (>0).
        @param comment the comment, or empty.
    */
    static EncodedSequence buildLoopLine(const juce::String& labelReference, int loopToIndex, const juce::String& comment);

    /** Sets the comment. */
    void setComment(const juce::String& comment) noexcept;

    /** @return whether this sequence is a loop command. */
    bool isLoopLine() const noexcept;

    /** Generates the data as source. */
    void generateSource(SourceGenerator& sourceGenerator) const noexcept;

    /** Equality operator. */
    bool operator==(const EncodedSequence&) const noexcept;
    /** Inequality operator. */
    bool operator!=(const EncodedSequence&) const noexcept;

    /** @return the size, in bytes, used to encode this sequence. */
    int getSize() const noexcept;

private:
    /** Constructor for one encoded byte. */
    EncodedSequence(int byte1, juce::String comment, juce::String labelReference, int loopToIndex) noexcept;
    /** Constructor for two encoded bytes. Warning, the endianness is not tested: the byte1 is encoded first. */
    EncodedSequence(int byte1, int byte2, juce::String comment, juce::String labelReference) noexcept;

    std::vector<int> bytes;                                     // The bytes of the encoded sequence. There must be at least one.
    juce::String labelReference;                                // A possible label to go to. As a simplification, it is always encoded at the end.
    int loopToLineIndex;                                        // If not -1, the line index the loop if jumping to. Should never be 0, as initial state can't be reached again.

    juce::String comment;                                       // A possible comment.
};

}   // namespace arkostracker
