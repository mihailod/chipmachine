#include "NoteUtil.h"

#include "NumberUtil.h"
#include "../song/cells/CellConstants.h"

namespace arkostracker 
{

const std::array<juce::String, NoteUtil::noteCountInOctave> NoteUtil::noteToString = {                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        "C-",
        "C#",
        "D-",
        "D#",
        "E-",
        "F-",
        "F#",
        "G-",
        "G#",
        "A-",
        "A#",
        "B-"
};

juce::String NoteUtil::getStringFromOctaveNote(const int noteInOctave) noexcept
{
    jassert(noteInOctave >= 0);
    return noteToString[static_cast<unsigned int>(noteInOctave) % noteCountInOctave];
}

juce::String NoteUtil::getStringFromNote(const int noteIndex) noexcept
{
    jassert((noteIndex >= CellConstants::minimumNote) && (noteIndex <= CellConstants::maximumNote));

    // Gets the base note without the octave (e.g. "C#").
    const auto baseNote = getStringFromOctaveNote(noteIndex);
    // Adds the octave.
    const auto octave = noteIndex / noteCountInOctave;
    return baseNote + NumberUtil::toHexDigit(octave);
}

juce::String NoteUtil::getStringFromNote(const Note note) noexcept
{
    return getStringFromNote(note.getNote());
}

int NoteUtil::getOctaveFromNote(int noteIndex) noexcept
{
    if (noteIndex >= 0) {
        return (noteIndex / noteCountInOctave);
    }
    // Negative. -1->-12 => -1, -13->-24 => -2, etc. Maybe some more elegant algorithm?
    noteIndex = -noteIndex - 1;
    return -(noteIndex / noteCountInOctave) - 1;
}

int NoteUtil::getNoteInOctave(const int noteIndex) noexcept
{
    const auto noteInOctave = std::abs(noteIndex % noteCountInOctave);      // Note in octave always positive.
    if (noteIndex >= 0) {
        // If positive, we can use the result as-is.
        return noteInOctave;
    }

    // If negative, the result is inverted in the octave.
    return (noteCountInOctave - noteInOctave) % noteCountInOctave;        // The mod takes care of the 12 case, put back to 0.
}

int NoteUtil::getNote(const int baseNote, const int octave) noexcept
{
    jassert(baseNote >= 0);
    return baseNote + octave * noteCountInOctave;
}

std::pair<int, int> NoteUtil::getNoteInOctaveAndOctave(const int noteIndex) noexcept
{
    jassert(noteIndex >= 0);
    return { noteIndex % 12, noteIndex / 12 };
}

}   // namespace arkostracker
