# Polytone file format (.POL) specification - version 1

The [Polytone](https://github.com/prochazkaml/Polytone) file format is based on the [Monotone](https://github.com/MobyGamer/MONOTONE) file format, with multiple enhancements:

- (Usually) smaller file sizes thanks to pattern packing
- Effect values are no longer limited to 0-63/0-7, now they can be from the full 0-255/0-15 range
- Expandable: Potential support for more effects & instruments in the future

## File header

A Polytone file starts with a header, which is mostly similar to the Monotone counterpart.

Offset|Length|Description (default value)
-|-|-
0x00|9 bytes|Signature ("\x08POLYTONE")
0x09|byte|Version number (0x01)
0x0A|byte|Number of patterns (1-256; if 0, then 256 is assumed)
0x0B|byte|Number of channels (1-16)
0x0C|byte|Number of entries in the order table (1-256; if zero, then 256 is assumed)
0x0D|[0x0C]|Order table (described below)

## Order table

The order table contains data, which the tracker uses to play the patterns in the correct order. Each byte of the order table contains a pattern ID (0-255).

## Pattern data

Next follows the actual pattern data. Each pattern is packed using a method that is very similar to the one used in the [Reality Adlib Tracker v2 file format](https://www.3eality.com/productions/reality-adlib-tracker/docs). It has to contain at least 1 row record.

### Row records

A row record can contain multiple channel entries.

*From now on, each element from the following tables is exactly 1 byte long.*

Offset|Description
-|-
0x00|**Row ID**<br>Bit 7: Last row record in the pattern<br>Bit 6: Row record is empty (see usage below)<br>Bits 0-5: Row number (0-63)

If a pattern is entirely empty (perhaps it is skipped), it contains only 1 byte, which has bits 6 and 7 set (0xC0-0xFF valid).

### Channel entry

Channel entries contain the actual note/effect data for each channel (that is not empty) in a given row record.

Offset|Description
-|-
0x00|**Channel descriptor**<br>Bit 7: Last channel entry in the row record<br>Bit 6: Note byte follows<br>Bit 5: Effect byte follows<br>Bit 4: *unused*<br>Bits 3-0: Channel number (0-15)
If bit 6 set: 0x01|**Note data**<br>Bit 7: *unused*<br>Bits 6-0: Note ID (0 = empty, 1-88 = A0-C8, 127 = OFF)
If bits 5 & 6 set: 0x02<br>If bit 5 set: 0x01|**Effect data**<br>Bits 7-4: Short effect value (0-13, 14 = use the same value as in the previous effect, 15 = read another byte for a full 8-bit value)<br>Bits 3-0: Effect type (see below in the Effect reference)
If bits 7-4 from the<br>previous byte == 15:<br>If bits 5 & 6 set: 0x03<br>If bit 5 set: 0x02|**Full-length effect value**<br>Bits 7-0: Effect value

### Effect reference

Effect ID|Requires a note|Description
-|-|-
0xy|after 1xx, 2xx or 3xx|**Arpeggio**<br>Quickly alternates between 3 notes at the tick frequency (default: 60 times a second).<br>The base note is always used on the first tick.<br>x = 0-15 semitones above the base note on the second tick<br>y = 0-15 semitones above the base note on the third tick<br>For example, `C-4 047` produces a C-major chord-like sound.
1xx|no|**Frequency slide up**<br>Increments the frequency of a playing channel by XX (1-255) Hz every tick on the row *except the first*.<br>For example, `C-4 10A` at the default speed (4) will result in a channel playing at 292 Hz, since the effect happens 3 times, so 262 Hz (C4) + 0xA * 3 = 292 Hz.
2xx|no|**Frequency slide down**<br>Decrements the frequency of a playing channel by XX Hz (1-255) every tick on the row *except the first*.<br>For example, `C-4 10A` at the default speed (4) will result in a channel playing at 232 Hz, since the effect happens 3 times, so 262 Hz (C4) - 0xA * 3 = 232 Hz.
3xx|first instance|**Frequency slide to note**<br>Increments/decrements the frequency of a playing channel by XX Hz (1-255) every tick on the row *except the first* towards a note supplied with the effect (it will not be actually played, it is just interpreted as the destination note).<br>If 0 Hz is supplied as the parameter, the effect will use the previous known value.<br>For example, the following block will fully slide from C4 (262 Hz) to C5 (523 Hz) by 48 Hz steps at speed 4:<br>`C-4 ---`<br>`C-5 330`<br>`--- 300`<br>
4xy|after 1xx, 2xx or 3xx|**Vibrato**<br>Oscillates around a note with a sinusoidal waveform.<br>x = speed 1-15 (from slowest to fastest, 0 = use the last known value)<br>y = depth 1-15 (from least effect (+/-6.25 cents) to most effect (+/-93.75 cents), 0 = use the last known value)<br>NOTE: Polytone's `466` is equivalent to Monotone's `433`. Polytone gives finer granularity over the effect rather than allowing for extreme values. `46F` therefore oscillates almost +/- one semitone (actually +/- 93.75 cents), closer than Monotone's `437` (which only oscillated +/- 87.5 cents).
Bxx|no|**Order jump**<br>Jumps to a position in the order table specified by xx.
Dxx|no|**Pattern break**<br>Jumps to the next position in the order table, to row xx.
Fxx|no|**Set speed**<br>Sets the playback speed. If xx <= 32, then it is interpreted as ticks per row, otherwise it is used as the playback frequency (33-255 Hz).
