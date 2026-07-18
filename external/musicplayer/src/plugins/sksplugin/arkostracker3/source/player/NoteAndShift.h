#pragma once

#include <utility>

namespace arkostracker 
{

/** Represents a relative note (maybe be negative) and a shift (signed). */
class NoteAndShift
{
public:
    NoteAndShift(int relativeNote, int shift) noexcept;

    /** @return the note in octave (0-11) and the octave. Works even for negative notes. */
    std::pair<int, int> getNoteInOctaveAndOctave() const;

    int getRelativeNote() const;
    int getShift() const;

private:
    int relativeNote;
    int shift;
};

}   // namespace arkostracker
