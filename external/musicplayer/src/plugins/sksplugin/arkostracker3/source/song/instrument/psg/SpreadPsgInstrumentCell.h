#pragma once

#include <set>

#include "LowLevelPsgInstrumentCell.h"
#include "PsgInstrumentCell.h"
#include "PsgSection.h"

namespace arkostracker 
{

/**
 * A Psg Instrument Cell with spread values *applied*, as well as the sections that use auto-spread.
 * It holds the cell by composition.
 */
class SpreadPsgInstrumentCell
{
public:
    /**
     * Constructor.
     * @param cellWithSpread the inner cell, with the spread values applied.
     * @param generatedSections the sections that are generated (because auto-spread).
     */
    SpreadPsgInstrumentCell(const PsgInstrumentCell& cellWithSpread, std::set<PsgSection> generatedSections) noexcept;

    /** @return a generated "low level" cell. */
    LowLevelPsgInstrumentCell buildLowLevelCell() const noexcept;

    /** @return a reference to the cell with auto-spread values applied. To know what are the sections which are generate, use the getGeneratedSectionsRef method. */
    const PsgInstrumentCell& getCellWithAutoSpreadAppliedRef() const noexcept;

    /** @return a Set containing the PsgSections that are generated (because of auto-spread). */
    const std::set<PsgSection>& getGeneratedSectionsRef() const noexcept;

    /** @return an empty cell. */
    static const SpreadPsgInstrumentCell& getEmptyCell() noexcept;

private:
    static const SpreadPsgInstrumentCell emptyCell;             // Useful for out-of-bounds cases.

    PsgInstrumentCell cellWithSpread;                           // This cell is generated: its data may be coming from auto-spread.
    std::set<PsgSection> generatedSections;                     // Contains the sections that are generated.
};

}   // namespace arkostracker
