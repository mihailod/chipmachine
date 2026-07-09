#pragma once

#include "../song/instrument/psg/PsgInstrumentCell.h"
#include "NoteAndShift.h"
#include "PsgPeriod.h"
#include "PsgRegisters.h"

namespace arkostracker 
{

class ChannelOutputRegisters;

/** Allows to read PsgRegisters, and converts them to other periods, or to PSG InstrumentCells. */
class PsgRegistersConverter     // TODO TU this (done in AT2).
{
public:
    /**
     * Constructor. The given registers are automatically converted.
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     * @param referenceFrequencyHz the target reference frequency, in Hz (440 most of the time).
     */
    PsgRegistersConverter(int sourcePsgFrequencyHz, int targetPsgFrequencyHz, float referenceFrequencyHz) noexcept;

    /**
     * Generates the PSG Instrument Cell from the PSG Registers.
     * @param registerList the registers to convert.
     * @param channelIndex the index of the channel to extract the Cell from.
     * @param encodeAsForcedPeriods true to encode forced periods, false to encode notes + shifts.
     * @return the PSG Instrument Cells.
     */
    std::vector<PsgInstrumentCell> encodeAsInstrumentCells(const std::vector<PsgRegisters>& registerList, int channelIndex, bool encodeAsForcedPeriods) noexcept;

    /**
     * @return the periods/noise according to the given PSG frequencies.
     * @param registers the input registers.
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     */
    static PsgRegisters convertPeriods(const PsgRegisters& registers, int sourcePsgFrequencyHz, int targetPsgFrequencyHz) noexcept;

private:
    static const int defaultRatio;
    static const int defaultHardwareEnvelope;
    static const int ratioShiftTolerance;                               // Arbitrary, a small shift is tolerated when finding ratios (s2h, h2s).
    static const int thresholdBenDaglishEffect;                         // Arbitrary, period below which a hardware period is considered a "Ben Daglish" effect.

    int sourcePsgFrequencyHz;                                           // The source frequencies, in Hz (2000000hz for Atari ST, for example).
    int targetPsgFrequencyHz;                                           // The target frequencies, in Hz (1000000hz for Atari ST, for example).
    PsgPeriod psgPeriod;
    int currentSoftwareBaseNote;                                        // The current software base note, or -1 if not known yet. Useful for note+shift generation.
    int currentHardwareBaseNote;                                        // The current software base note, or -1 if not known yet. Useful for note+shift generation.

    /**
     * Converts a software period from one PSG frequency to another.
     * @param period the period.
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     * * @return the converted period.
     */
    static int convertSoftwarePeriod(int period, int sourcePsgFrequencyHz, int targetPsgFrequencyHz) noexcept;

    /**
     * Converts a hardware period from one PSG frequency to another.
     * @param period the period.
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     * @return the converted period.
     */
    static int convertHardwarePeriod(int period, int sourcePsgFrequencyHz, int targetPsgFrequencyHz) noexcept;

    /**
     * Converts a noise from one PSG frequency to another.
     * @param noise the noise (>=0).
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     * @return the converted noise.
     */
    static int convertNoise(int noise, int sourcePsgFrequencyHz, int targetPsgFrequencyHz) noexcept;

    /**
     * Converts a period (software or hardware) from one PSG frequency to another. If 0, remains at 0.
     * @param period the period (>=0).
     * @param maximumValue the maximum value the period can reach (0xfff for software period, 0xffff for hardware period, for example).
     * @param sourcePsgFrequencyHz the source frequencies, in Hz (2000000hz for Atari ST, for example).
     * @param targetPsgFrequencyHz the target frequencies, in Hz (1000000hz for Atari ST, for example).
     * @return the converted period.
     */
    static int convertPeriod(int period, int maximumValue, int sourcePsgFrequencyHz, int targetPsgFrequencyHz) noexcept;

    /** @return the PSG Instrument Cell, with forced periods, from the given ChannelRegisters. */
    static PsgInstrumentCell encodeAsCellForcePeriods(const ChannelOutputRegisters& channelRegisters) noexcept;
    /** @return the PSG Instrument Cell, with notes and shifts, from the given ChannelRegisters. */
    PsgInstrumentCell encodeAsCellNotesAndShifts(const ChannelOutputRegisters& channelRegisters) noexcept;

    /**
     * Returns the note and shift to encode, according to the given period (software or hardware), and the current base note, if any.
     * If there is none, the found note becomes the base note.
     * @param period the period (software or hardware).
     * @param software true for software, false for hardware.
     * @return the note and shift to encode.
     */
    NoteAndShift findNoteAndShiftToEncode(int period, bool software) noexcept;

    /**
     * Generates a hardware PSG Instrument Cell from the given channel registers, which must be of any software+hardware type.
     * @param registers the registers. Both software and hardware are used.
     * @return the cell.
     */
    PsgInstrumentCell generateInstrumentCellFromSoftwareAndHardwareChannelRegisters(const ChannelOutputRegisters& registers) noexcept;

    /**
     * Finds the Cell Link between the soft and hard of the given registers.
     * @param registers the registers.
     * @return the link, plus the ratio (the ratio is not useful for SoftAndHard).
     */
    std::pair<PsgInstrumentCellLink, int> findLink(const ChannelOutputRegisters& registers) noexcept;

    /**
     * Finds the ratio (in AT2 terminology) between the software and hardware periods. A slight shift is tolerated.
     * @param softwarePeriod the software period.
     * @param hardwarePeriod the hardware period.
     * @return the ratio (0-7), or -1 if it couldn't be found, plus a shift.
     */
    static std::pair<int, int> findRatioAndShiftSoftToHard(int softwarePeriod, int hardwarePeriod) noexcept;

    /**
     * Finds the ratio (in AT2 terminology) between the hardware and software periods. A slight shift is tolerated.
     * @param softwarePeriod the software period.
     * @param hardwarePeriod the hardware period.
     * @return the ratio (0-7), or -1 if it couldn't be found, plus a shift.
     */
    static std::pair<int, int> findRatioAndShiftHardToSoft(int softwarePeriod, int hardwarePeriod) noexcept;
};

}   // namespace arkostracker

