#pragma once

#include "CellEffects.h"
#include "Note.h"

namespace arkostracker 
{

/** A Cell for "music" Tracks. */
class Cell
{
public:
    /** Constructor of an empty cell. */
    Cell() noexcept;

    /**
     * Constructor.
     * @param note the possible note.
     * @param instrument the possible instrument.
     */
    Cell(const OptionalValue<Note>& note, const OptionalInt& instrument) noexcept;

    /**
     * Constructor.
     * @param note the possible note.
     * @param instrument the possible instrument.
     * @param effects the effects.
     */
    Cell(const OptionalValue<Note>& note, const OptionalInt& instrument, CellEffects effects) noexcept;

    /** Simple builder with a present note and instrument. */
    static Cell build(int note, int instrument) noexcept;

    /** Builder with a present note and instrument, and CellEffects. */
    static Cell build(int note, int instrument, const CellEffects& effects) noexcept;

    /** Builder for a RST. */
    static Cell buildRst() noexcept;

    /**
     * Constructor with only effects.
     * @param effects the effects.
     */
    explicit Cell(CellEffects effects) noexcept;

    /** Indicates whether the Cell is empty or not. */
    bool isEmpty() const noexcept;
    /** @return true if there is no note and instrument, no effect is present, but also if the raw value is 0. */
    bool isClean() const noexcept;

    /** @return a reference to the possible note. */
    const OptionalValue<Note>& getNote() const noexcept;
    /**
     * Sets the note.
     * @param note the note, but may be empty.
     */
    void setNote(OptionalValue<Note> note) noexcept;

    /** @return a reference to the possible instrument. */
    const OptionalInt& getInstrument() const noexcept;
    /**
     * Sets the instrument.
     * @param instrumentIndex the instrument index, but may be empty (becomes a legato if there is a note).
     */
    void setInstrument(OptionalInt instrumentIndex) noexcept;

    /** @return a reference to the effects. */
    const CellEffects& getEffects() const noexcept;
    /**
     * Sets the whole Cell Effects. Mostly useful for builders, etc.
     * @param cellEffects the Cell Effects.
     */
    void setEffects(const CellEffects& cellEffects) noexcept;

    /**
     * Sets a digit of an effect. If invalid, nothing happens but asserts.
     * @param effectIndex the index of the effect (0-3).
     * @param digitIndex the digit index (0-2).
     * @param value the value (0-15).
     */
    void setEffectDigit(int effectIndex, int digitIndex, int value) noexcept;

    /**
     * Sets an effect number. If invalid, nothing happens but asserts.
     * @param effectIndex the index of the effect (0-3).
     * @param effect the effect.
     */
    void setEffectNumber(int effectIndex, Effect effect) noexcept;

    /**
     * @return true if an effect is present (the value is not tested).
     * @param effectIndex the effect index.
     */
    bool isEffectPresent(int effectIndex) const noexcept;

    /**
     * Mixes the current Cell with a new one, which overwrites only the non-empty fields it has.
     * @param cellWithPriority the Cell which data has priority, but if absent, won't be used.
     * @return the mixed Cell.
     */
    Cell mix(const Cell& cellWithPriority) const noexcept;

    /** @return true if there is a note, but no instrument and no glide effect. */
    bool isLegato() const noexcept;

    /** @return true if a note is present (the presence of the instrument is not tested). */
    bool isNote() const noexcept;
    /** @return true if an instrument (may be RST). */
    bool isInstrument() const noexcept;
    /** @return true if both a note and instrument are present (may be RST). */
    bool isNoteAndInstrument() const noexcept;
    /** @return true if there is a RST (both a note and the instrument must be present). The note value can be anything. */
    bool isRst() const noexcept;

    /** Indicates whether there are effects (other than "no effects"). "No effects" with values are ignored. */
    bool hasEffects() const noexcept;

    /** Clears the note. */
    void clearNote() noexcept;
    /** Clears the instrument. */
    void clearInstrument() noexcept;
    /** Clears the whole content. */
    void clear() noexcept;

    /**
     * Adds an effect, where there is room.
     * @param effect the Effect.
     * @param logicalValue the logical value (no need to shift).
     * @return false if too many effects and nothing has been added.
     */
    bool addEffect(Effect effect, int logicalValue) noexcept;

    /**
     * Sets an effect. If already present, it is overwritten. If the index is out of bounds, returns false.
     * @param effectIndex the effect index. May be invalid.
     * @param cellEffect the cell effect.
     * @return false if the index out of bounds and nothing has been added.
     */
    bool setEffect(int effectIndex, const CellEffect& cellEffect) noexcept;

    /**
    * @return the CellEffect if the effect is found. They are browsed in order.
    * @param effect the effect to find.
    */
    OptionalValue<CellEffect> find(Effect effect) const noexcept;

    /**
     * @return the CellEffect if the effect is found and its index (or -1). They are browsed in order.
     * @param effect the effect to find.
     */
    std::pair<int, OptionalValue<CellEffect>> findWithIndex(Effect effect) const noexcept;

    /**
     * Browses all the effects. It may be an empty effect. The function is called, with the index and the CellEffect.
     * It is possible to modify the CellEffect.
     * @param operationToApply the operation to apply.
     */
    void performOnEffects(const std::function<void(std::pair<int, CellEffect&>)>& operationToApply) noexcept;

    /**
     * Removes the effect and the value.
     * @param effectIndex the effect index. May be invalid.
     */
    void removeEffectFromIndex(int effectIndex) noexcept;

    /**
     * Removes the first effect and value which matches.
     * @param effect the effect.
     * @return true if found and removed.
     */
    bool removeEffect(Effect effect) noexcept;

    /**
     * Sets the value of an effect.
     * @param effectIndex the effect index. May be invalid.
     * @param rawValue the raw value (3 digits).
     */
    void setEffectValue(int effectIndex, int rawValue) noexcept;

    /**
     * Shifts the effect, in any direction.
     * @param shift positive to shift "to the right".
     */
    void shiftEffects(int shift) noexcept;

    /**
     * Corrects a note, so that it doesn't go out of boundaries. However, to emulate what is done on Z80,
     * it loops rather than being maximized or minimized.
     * @param note the note to correct.
     * @return the note, corrected if necessary.
     */
    static int correctNote(int note) noexcept;

    /**
     * Builds a new Cell which is the current one, with another Cell which will overwrite some parts of it.
     * @param additionalCell the cell that will overwrite the current one.
     * @param overwriteNote true for the additional Cell to overwrite the current note.
     * @param overwriteInstrument true for the additional Cell to overwrite the current instrument.
     * @param overwriteEffect1 true for the additional Cell to overwrite the current effect 1.
     * @param overwriteEffect2 true for the additional Cell to overwrite the current effect 2.
     * @param overwriteEffect3 true for the additional Cell to overwrite the current effect 3.
     * @param overwriteEffect4 true for the additional Cell to overwrite the current effect 4.
     * @return a new Cell.
     */
    Cell with(const Cell& additionalCell, bool overwriteNote, bool overwriteInstrument, bool overwriteEffect1, bool overwriteEffect2,
              bool overwriteEffect3, bool overwriteEffect4) const noexcept;

    /**
     * Builds a new Cell with a new Instrument.
     * @param instrumentIndex the new Instrument.
     */
    Cell withInstrument(int instrumentIndex) const noexcept;

    /**
     * Indicates whether effects are present, and if yes, they are only among the given ones.
     * @param effects the effects we are looking for. Must NOT contain the No Effect!
     * @return true if at least one of the given effect is present.
     */
    bool areEffectsPresentAmong(const std::set<Effect>& effects) const noexcept;

    /** Equality operator. The cell must be EXACTLY the same. */
    bool operator==(const Cell& rhs) const noexcept;
    bool operator!=(const Cell& rhs) const noexcept;

private:
    OptionalValue<Note> note;               // A possible note.
    OptionalInt instrument;                 // The index of the instrument.

    CellEffects effects;                    // The effects.
};

}   // namespace arkostracker

