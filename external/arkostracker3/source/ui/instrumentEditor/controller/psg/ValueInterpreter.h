#pragma once

#include <memory>
#include <unordered_map>

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"

namespace arkostracker 
{

/** Holds some logic about the values in the Bars. */
class ValueInterpreter          // TODO TU this.
{
public:

    /**
     * Creates a new Cell, if the input value for a Link (type) of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForTypeChange(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a Noise of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForNoiseChange(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for an envelope of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForEnvelopeChange(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;


    // ------------------------------------------------------------

    /**
     * Creates a new Cell, if the input value for a Primary Period of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPrimaryPeriodChange(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a Primary Arpeggio Note In Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPrimaryArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a Primary Arpeggio Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPrimaryArpeggioOctave(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a Primary Pitch of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPrimaryPitch(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;


    // ------------------------------------------------------------

    /**
     * Creates a new Cell, if the input value for a Secondary Period of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSecondaryPeriodChange(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;
    
    /**
     * Creates a new Cell, if the input value for a Secondary Arpeggio Note In Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSecondaryArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a Secondary Arpeggio Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSecondaryArpeggioOctave(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;
    
    /**
     * Creates a new Cell, if the input value for a Secondary Pitch of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSecondaryPitch(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    // ------------------------------------------------------------

    /**
     * Creates a new Cell, if the input value for a SID Is Activated and Ratio of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSidIsActivatedAndRatio(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a SID balance percent of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSidBalancePercent(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a SID low shelf volume of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSidLowShelfVolume(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a SID arpeggio of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSidArpeggio(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    /**
     * Creates a new Cell, if the input value for a SID pitch of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. May be out of range.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForSidPitch(const PsgInstrumentCell& originalCell, int inputValue, bool forceGeneration) noexcept;

    // ------------------------------------------------------------

    /**
     * Determines the raw int value to send to an Envelope Bar, according to the given Cell.
     * @param cell the Cell.
     * @return the value.
     */
    static int determineEnvelopeUiValueFromCell(const PsgInstrumentCell& cell) noexcept;

    /** @return the value to put in a Bar, from the given Link. */
    static int getLinkToBarValue(PsgInstrumentCellLink link) noexcept;

    /**
     * @return true if the given hardware envelope is among the most useful.
     * @param hardwareEnvelope the hardware envelope. Should be from 8-15.
     */
    static bool isMostUsefulHardwareEnvelope(int hardwareEnvelope) noexcept;

private:
    /** A map between a Link and its value in a bar. */
    static const std::unordered_map<PsgInstrumentCellLink, int> linkToBarValue;

    /** Links a bar value envelope (0-7) to a Cell envelope (8-15). The order is not linear. */
    static const std::unordered_map<int, int> barValueEnvelopeToValidEnvelope;

    /**
     * Generic method to create a new Cell, if the input value for a primary/secondary Period of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. Should be within range.
     * @param getPeriodLambda lambda to get the period from the cell.
     * @param createCellLambda lambda to create the Cell with the new period.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPeriodChange(const PsgInstrumentCell& originalCell, int inputValue,
            const std::function<int(const PsgInstrumentCell& originalCell)>& getPeriodLambda,
            const std::function<PsgInstrumentCell(const PsgInstrumentCell& originalCell, int period)>& createCellLambda, bool forceGeneration) noexcept;

    /**
     * Generic method to create a new Cell, if the input value for a primary/secondary Arpeggio Note In Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. Should be within range.
     * @param primary true if primary, false if secondary.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, int inputValue, bool primary, bool forceGeneration) noexcept;

    /**
     * Generic method to creates a new Cell, if the input value for a primary/secondary Arpeggio Octave of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. Should be within range.
     * @param primary true if primary, false if secondary.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForArpeggioOctave(const PsgInstrumentCell& originalCell, int inputValue, bool primary, bool forceGeneration) noexcept;

    /**
     * Generic method to create a new Cell, if the input value for a primary/secondary Pitch of a bar is different.
     * @param originalCell the original Cell.
     * @param inputValue the value in the bar. Should be within range.
     * @param primary true if primary, false if secondary.
     * @param forceGeneration true to force the generation, even if no modification.
     * @return the modified Cell, or nullptr if not changed.
     */
    static std::unique_ptr<PsgInstrumentCell> createCellForPitch(const PsgInstrumentCell& originalCell, int inputValue, bool primary, bool forceGeneration) noexcept;
};


}   // namespace arkostracker

