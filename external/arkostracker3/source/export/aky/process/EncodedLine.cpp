#include "EncodedLine.h"

#include "../../sourceGenerator/SourceGenerator.h"

namespace arkostracker 
{

EncodedLine::EncodedLine() noexcept:
        encodedSequences()
{
}

EncodedLine::EncodedLine(const EncodedSequence& sequence) noexcept:
        encodedSequences()
{
    addSequence(sequence);
}

EncodedLine::EncodedLine(const EncodedSequence& sequence1, const EncodedSequence& sequence2) noexcept:
        encodedSequences()
{
    addSequence(sequence1);
    addSequence(sequence2);
}

void EncodedLine::addSequence(const EncodedSequence& sequence) noexcept
{
    encodedSequences.push_back(sequence);
}

void EncodedLine::insertSequencesAtBeginning(const std::vector<EncodedSequence>& sequencesToAdd) noexcept
{
    encodedSequences.insert(encodedSequences.begin(), sequencesToAdd.cbegin(), sequencesToAdd.cend());
}

void EncodedLine::generateSource(SourceGenerator& sourceGenerator) const noexcept
{
    for (const auto& encodedSequence : encodedSequences) {
        encodedSequence.generateSource(sourceGenerator);
    }
    sourceGenerator.addEmptyLine();
}

const std::vector<EncodedSequence>& EncodedLine::getEncodedSequences() const
{
    return encodedSequences;
}

int EncodedLine::getSize() const noexcept
{
    auto size = 0;

    // Simply browses the Sequences and adds their size.
    for (const auto& encodedSequence : encodedSequences) {
        size += encodedSequence.getSize();
    }
    jassert(size > 0);      // There shouldn't be an empty line.

    return size;
}

bool EncodedLine::isEmpty() const noexcept
{
    return encodedSequences.empty();
}

bool EncodedLine::operator==(const EncodedLine& other) const noexcept
{
    return (encodedSequences == other.encodedSequences);    // Metadata not taken in account, no need.
}

bool EncodedLine::operator!=(const EncodedLine& other) const noexcept
{
    return !operator==(other);
}

}   // namespace arkostracker

