#pragma once

#include <vector>

#include "EncodedSequence.h"

namespace arkostracker 
{

class SourceGenerator;

/**
 * Represents a line to be encoded (a line is a "frame" for one channel). This is useful to allow comparison of raw data before (!) encoding it.
 * This allows finding some new optimizations from raw bytes.
 *
 * One Line can be composed of one or several EncodedSequence.
 */
class EncodedLine
{
public:
    /** Constructor. */
    explicit EncodedLine() noexcept;

    /** Constructor for one sequence. */
    explicit EncodedLine(const EncodedSequence& sequence) noexcept;
    /** Constructor for two sequences. */
    EncodedLine(const EncodedSequence& sequence1, const EncodedSequence& sequence2) noexcept;

    /** Adds a sequence. */
    void addSequence(const EncodedSequence& sequence) noexcept;
    /** Adds sequences at the beginning. */
    void insertSequencesAtBeginning(const std::vector<EncodedSequence>& sequencesToAdd) noexcept;

    /** Generates the source for this line. */
    void generateSource(SourceGenerator& sourceGenerator) const noexcept;

    /** @return the encoded sequences for this line. */
    const std::vector<EncodedSequence>& getEncodedSequences() const;

    /** Equality operator. */
    bool operator==(const EncodedLine&) const noexcept;
    /** Inequality operator. */
    bool operator!=(const EncodedLine&) const noexcept;

    /** @return the size, in bytes, used to encode this line. */
    int getSize() const noexcept;

    /** @return true if there is no data. */
    bool isEmpty() const noexcept;

private:
    std::vector<EncodedSequence> encodedSequences;                          // The sequences this line is composed of.
};

}   // namespace arkostracker

