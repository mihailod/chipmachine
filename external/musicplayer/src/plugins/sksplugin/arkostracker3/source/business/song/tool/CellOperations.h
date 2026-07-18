#pragma once

#include "../../../song/cells/Cell.h"

namespace arkostracker 
{

/** Converts a legato note to an effect. The note is removed. */
class LegatoToEffect
{
public:
    /**
     * If there is a note without Instrument AND no glide effect, this removes the note and adds a Legato effect.
     * Note that the effect is added where possible, and thus may not be correct. The Cell should be normalized after.
     * @param cell the Cell to read.
     * @return nullptr if nothing needs to be done, else the modified Cell.
     */
    std::unique_ptr<Cell> operator()(const Cell& cell) const noexcept;
};

class NormalizeCell
{
public:
    /**
     * Clears the unused data of a Cell, corrects the effects and normalize their order.
     * Also, if using instrument 0, forces the note to a "convention" RST note.
     * @param cell the Cell to read.
     * @return nullptr if nothing needs to be done, else the modified Cell.
     */
    std::unique_ptr<Cell> operator()(const Cell& cell) const noexcept;
};

class IllegalInstrumentToRstInCell
{
public:
    /**
     * Constructor.
     * @param instrumentCount how many Instruments in the Song. Must be >0.
     */
    explicit IllegalInstrumentToRstInCell(int instrumentCount) noexcept;

    /**
     * Replaces all illegal Instruments to RST (with the note being also changed).
     * @param cell the Cell to read.
     * @return nullptr if nothing needs to be done, else the modified Cell.
     */
    std::unique_ptr<Cell> operator()(const Cell& cell) const noexcept;

private:
    const int instrumentCount;
};

class IllegalExpressionTo0InCell
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionCount how many Expression (of target type) in the Song. Must be >0.
     */
    explicit IllegalExpressionTo0InCell(bool isArpeggio, int expressionCount) noexcept;

    /**
     * Replaces all illegal Expression reference to 0.
     * @param cell the Cell to read.
     * @return nullptr if nothing needs to be done, else the modified Cell.
     */
    std::unique_ptr<Cell> operator()(const Cell& cell) const noexcept;

private:
    const bool isArpeggio;
    const int expressionCount;
};

}   // namespace arkostracker
