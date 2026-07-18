#include "Note.h"

#include <juce_core/juce_core.h>

#include "CellConstants.h"

namespace arkostracker 
{

Note::Note() noexcept :
        note(12 * 4)            // Any note will do, this will not be used.
{
}

Note::Note(const int pNote) noexcept :
        note(pNote)
{
    jassert(((pNote >= CellConstants::minimumNote) && (pNote <= CellConstants::maximumNote)) || (pNote == CellConstants::rstNote));
}

Note Note::buildNote(const int note) noexcept
{
    return Note(note);
}

Note Note::buildRst() noexcept
{
    return Note(CellConstants::rstNote);
}

int Note::getNote() const noexcept
{
    return note;
}

bool Note::operator==(const Note& rhs) const             // NOLINT(fuchsia-overloaded-operator)
{
    return (note == rhs.note);
}

bool Note::operator!=(const Note& rhs) const             // NOLINT(fuchsia-overloaded-operator)
{
    return !(rhs == *this);
}

}   // namespace arkostracker

