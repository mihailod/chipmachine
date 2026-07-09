#pragma once

#include <array>

#include <juce_core/juce_core.h>

#include "../song/cells/Note.h"

namespace arkostracker 
{

/**
 * Utility class to get displayable notes from a note.
 */
class NoteUtil
{
public:
    /** Prevents instantiation. */
    NoteUtil() = delete;

    /** How many notes in an octave. */
    static constexpr auto noteCountInOctave = 12;

    static const std::array<juce::String, static_cast<unsigned int>(noteCountInOctave)> noteToString;           // The notes of an octave, to the String to display.

    /**
     * Returns the String related to the note in one octave (such as "C#" or "A-").
     * @param noteInOctave the note. Any octave, or none, will do. Must be >=0.
     * @return the String.
     */
    static juce::String getStringFromOctaveNote(int noteInOctave) noexcept;

    /**
     * Returns the String (note and octave) related to the given note (such as "C#4").
     * @param noteIndex the note. It should stay within the 10 octaves range, else the display will be weird. Must be >=0.
     * @return the String.
     */
    static juce::String getStringFromNote(int noteIndex) noexcept;

    /**
     * Returns the String (note and octave) related to the given note. Cannot be a RST too.
     * @param note the note. It should stay within the 10 octaves range, else the display will be weird.
     * @return the String.
     */
    static juce::String getStringFromNote(Note note) noexcept;

    /**
     * @return the octave from the note number (example 0 for 0-11, 1 for 12-23, etc.).
     * @param noteIndex the note number (may be negative, useful for arpeggio).
     */
    static int getOctaveFromNote(int noteIndex) noexcept;

    /**
     * @return the note in an octave from the note number (example 0 for 12, 1 for 13, etc.). Always positive.
     * @param noteIndex the note number (may be negative).
     */
    static int getNoteInOctave(int noteIndex) noexcept;

    /**
     * @return the note, from the base note and its octave.
     * @param baseNote should be >=0. Can be more than an octave.
     * @param octave the octave, but may be negative.
     */
    static int getNote(int baseNote, int octave) noexcept;

    /**
     * @return the note in octave (0-11) and the octave.
     * @param noteIndex the note index (0-127 or more (no check)).
     */
    static std::pair<int, int> getNoteInOctaveAndOctave(int noteIndex) noexcept;
};

}   // namespace arkostracker
