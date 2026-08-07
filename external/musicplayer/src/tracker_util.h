#pragma once

// Small helpers for filling in musix::TrackerCell from engines that hand out
// raw pattern bytes (libxmp, the AHX/HVL replayer, ...). libopenmpt formats its
// own cells and does not need any of this.

#include "chipplayer.h"

#include <cstdint>
#include <cstring>

namespace musix::tracker {

// "C-", "C#", "D-", ... indexed by semitone within the octave.
inline const char* noteLetters(int semi)
{
    static const char* names[12] = { "C-", "C#", "D-", "D#", "E-", "F-",
                                     "F#", "G-", "G#", "A-", "A#", "B-" };
    return names[semi % 12];
}

// semitone 0 == C-0. Octaves outside 0..9 are clamped so the string always
// stays three characters wide and the columns stay aligned.
inline void setNote(TrackerCell& cell, int semitone)
{
    if (semitone < 0) { semitone = 0; }
    int oct = semitone / 12;
    if (oct > 9) { oct = 9; }
    const char* l = noteLetters(semitone);
    cell.note[0] = l[0];
    cell.note[1] = l[1];
    cell.note[2] = static_cast<char>('0' + oct);
    cell.note[3] = 0;
}

inline void setNoteText(TrackerCell& cell, const char* text)
{
    std::strncpy(cell.note, text, sizeof(cell.note) - 1);
}

inline char hexDigit(int v)
{
    return "0123456789ABCDEF"[v & 0xf];
}

// Instrument/sample number, two hex digits. 0 means "none" in every format that
// uses this helper, so it leaves the cell empty.
inline void setInstrument(TrackerCell& cell, int ins)
{
    if (ins <= 0) { return; }
    cell.inst[0] = hexDigit(ins >> 4);
    cell.inst[1] = hexDigit(ins);
    cell.inst[2] = 0;
}

// One effect command: a single-character type followed by a two-hex-digit
// parameter. Types above 0xf (libxmp's internal, non-MOD effects) continue into
// the letters, which is the convention every tracker UI uses.
inline void setEffect(TrackerCell& cell, int type, int param)
{
    if (type == 0 && param == 0) { return; }
    cell.fx[0] = type < 16 ? hexDigit(type)
                           : static_cast<char>('A' + ((type - 16) % 26));
    cell.fx[1] = hexDigit(param >> 4);
    cell.fx[2] = hexDigit(param);
    cell.fx[3] = 0;
}

} // namespace musix::tracker
