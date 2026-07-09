#include "EncodedSequence.h"

#include "../../sourceGenerator/SourceGenerator.h"
#include "AkyConstants.h"

namespace arkostracker 
{

EncodedSequence::EncodedSequence(const int pByte1, juce::String pComment, juce::String pLabelReference, const int pLoopToIndex) noexcept :
    bytes(),
    labelReference(std::move(pLabelReference)),
    loopToLineIndex(pLoopToIndex),
    comment(std::move(pComment))
{
    bytes.push_back(pByte1);
}

EncodedSequence::EncodedSequence(const int pByte1, const int pByte2, juce::String pComment, juce::String pLabelReference) noexcept :
    bytes(),
    labelReference(std::move(pLabelReference)),
    loopToLineIndex(-1),
    comment(std::move(pComment))
{
    bytes.push_back(pByte1);
    bytes.push_back(pByte2);
}

EncodedSequence EncodedSequence::buildOneByte(int byte1, const juce::String& comment)
{
    jassert((byte1 >= 0) && (byte1 <= 0xff));
    return { byte1, comment, juce::String(), -1 };
}

EncodedSequence EncodedSequence::buildWord(const int w, const juce::String& comment, const bool littleEndian)
{
    jassert((w >= 0) && (w <= 0xffff));
    const auto byte0 = (w & 0xff);
    const auto byte1 = ((w / 256) & 0xff);
    // Encodes as little-endian?
    if (littleEndian) {
        return { byte0, byte1, comment, juce::String() };
    }
    return { byte1, byte0, comment, juce::String() };
}

EncodedSequence EncodedSequence::buildLoopLine(const juce::String& labelReference, int loopToIndex, const juce::String& comment)
{
    jassert(loopToIndex != 0);      // No loop on the initial state is allowed!
    return { AkyConstants::loopByte, comment, labelReference, loopToIndex };
}

void EncodedSequence::setComment(const juce::String& newComment) noexcept
{
    comment = newComment;
}

void EncodedSequence::generateSource(SourceGenerator& sourceGenerator) const noexcept
{
    jassert(!bytes.empty());        // The bytes shouldn't be empty.

    // Encodes the bytes, and the comment.
    for (const auto b : bytes) {
        sourceGenerator.declareByte(b, comment);
    }

    // If a label is present, encodes it.
    if (!labelReference.isEmpty()) {
        sourceGenerator.declarePointerRegionStart();
        sourceGenerator.declareWord(labelReference);
        sourceGenerator.declarePointerRegionEnd();
    }
}

bool EncodedSequence::isLoopLine() const noexcept
{
    return (loopToLineIndex >= 0);
}

bool EncodedSequence::operator==(const EncodedSequence& other) const noexcept
{
    // Line index is ok. What about the rest?
    return ((bytes == other.bytes) && (labelReference == other.labelReference));
}

bool EncodedSequence::operator!=(const EncodedSequence& other) const noexcept
{
    return !operator==(other);
}

int EncodedSequence::getSize() const noexcept
{
    // The size is the bytes, plus the possible Goto label (a word).
    return static_cast<int>(bytes.size()) + (isLoopLine() ? 2 : 0);
}

}   // namespace arkostracker
