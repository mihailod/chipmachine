#include "NoteAndShift.h"

#include "../utils/NoteUtil.h"

namespace arkostracker 
{

NoteAndShift::NoteAndShift(const int pRelativeNote, const int pShift) noexcept :
        relativeNote(pRelativeNote),
        shift(pShift)
{
}

int NoteAndShift::getRelativeNote() const
{
    return relativeNote;
}

int NoteAndShift::getShift() const
{
    return shift;
}

std::pair<int, int> NoteAndShift::getNoteInOctaveAndOctave() const
{
    return { NoteUtil::getNoteInOctave(relativeNote), NoteUtil::getOctaveFromNote(relativeNote) };
}

}   // namespace arkostracker
