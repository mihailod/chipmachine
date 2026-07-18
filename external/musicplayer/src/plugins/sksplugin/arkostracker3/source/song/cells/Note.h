#pragma once



#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Represents a note, or a RST. */
class Note
{
public:
    /** A default constructor, required for OptionalValue. Builds a C-4 note, but it shouldn't matter. */
    Note() noexcept;

    /** @return a note. */
    static Note buildNote(int note) noexcept;

    /** @return a RST note. The note is only a convention and shouldn't tested. */
    static Note buildRst() noexcept;

    /** @return the note. May be a RST (see CellConstants). */
    int getNote() const noexcept;

    bool operator==(const Note& rhs) const;             // NOLINT(fuchsia-overloaded-operator)
    bool operator!=(const Note& rhs) const;             // NOLINT(fuchsia-overloaded-operator)

private:
    /**
     * Builds a note.
     * @param note the note from minimumNote to maximumNote.
     */
    explicit Note(int note) noexcept;

    int note;              // Warning, can be a RST!
};

}   // namespace arkostracker

