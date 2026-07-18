#pragma once

#include <unordered_map>
#include <vector>

#include "../../../business/model/Loop.h"
#include "LowLevelPsgInstrumentCell.h"
#include "PsgInstrumentCell.h"
#include "SpreadPsgInstrumentCell.h"

namespace arkostracker 
{

/** This is the PSG part of an Instrument (as opposed to Sample). It also contains the data of sound effects. */
class PsgPart
{
public:
    /** Constructor. This build an invalid PSG Part, because it is empty. */
    PsgPart() noexcept;

    /** @return the speed. */
    int getSpeed() const noexcept;
    /**
     * Sets the speed.
     * @param speed the speed (>=0, <=255).
     */
    void setSpeed(int speed) noexcept;

    /**
     * Sets the retrig, to be played at the beginning of the Instrument.
     * @param retrig true to Retrig.
     */
    void setInstrumentRetrig(bool retrig) noexcept;
    /** @return true if retrig at the beginning of the Instrument. */
    bool isInstrumentRetrig() const noexcept;

    /**
     * Adds a Cell at the end. This does not update the loop.
     * @param cell the Cell.
     */
    void addCell(const PsgInstrumentCell& cell) noexcept;

    /**
     * Inserts Cells. This does not update the loop.
     * @param cellIndex the index of the Cell to add. Must be valid.
     * @param cell the Cell.
     */
    void insertCell(int cellIndex, const PsgInstrumentCell& cell) noexcept;

    /**
     * Removes Cells. This does not update the loop, which should be checked before or after (in a transaction).
     * As a convenience, nothing is done if the cell count is 0.
     * @param cellIndex the index of the Cell to remove. Must be valid.
     * @param cellCount how many cells to remove. Must be valid.
     */
    void removeCells(int cellIndex, int cellCount) noexcept;

    /**
     * @return a reference to a Cell. This method is safe. If the Cell does not exist, a reference to an empty Cell is returned.
     * @param cellIndex the cell index. May be invalid.
     */
    const PsgInstrumentCell& getCellRefConst(int cellIndex) const noexcept;

    /**
     * Allows to perform an action on each Cell (up to the full end).
     * @param function what to perform, on the cell index and cell itself. It is safe to call setCell to replace it if wanted.
     */
    void performOnEachCell(const std::function<void(int, const PsgInstrumentCell&)>& function) const noexcept;

    /**
     * @return a generated low level PsgCell, useful for players, which has auto-spread applied.
     * This method is safe. If the Cell does not exist, an empty Cell is returned.
     * This method is only a shortcut for buildSpreadPsgInstrumentCell then buildLowLevelCell method.
     * @param cellIndex the cell index. May be invalid.
     */
    LowLevelPsgInstrumentCell buildLowLevelCell(int cellIndex) const noexcept;

    /** @return how many Cells there are. */
    int getLength() const noexcept;

    /** @return the loop data. */
    Loop getMainLoop() const noexcept;
    /** @return the loop data, as a reference. Use this is a transaction. */
    const Loop& getMainLoopRef() const noexcept;

    /**
     * Sets the new loop data.
     * @param loop the loop.
     */
    void setMainLoop(const Loop& loop) noexcept;

    /**
     * Sets a Cell.
     * @param cellIndex the cell index. If invalid, nothing happens.
     * @param cell the new cell.
     */
    void setCell(int cellIndex, const PsgInstrumentCell& cell) noexcept;

    /**
     * @return true if the auto-spread for the section is on.
     * @param psgSection the PSG Section.
     */
    bool isAutoSpread(PsgSection psgSection) const noexcept;

    /**
     * @return the auto-spread Loop used for the given Section.
     * @param psgSection the section.
     */
    Loop getAutoSpreadLoop(PsgSection psgSection) const noexcept;

    /**
     * @return a reference to the auto-spread Loop used for the given Section. Should be used only in a transaction.
     * @param psgSection the section.
     */
    const Loop& getAutoSpreadLoopRef(PsgSection psgSection) const noexcept;

    /**
     * Sets the auto-spread loop for a Section.
     * @param psgSection the Section.
     * @param autoSpreadLoop the new loop.
     */
    void setAutoSpreadLoop(PsgSection psgSection, const Loop& autoSpreadLoop) noexcept;

    /** @return a map linking all the sections to their auto-spread loops. */
    std::unordered_map<PsgSection, Loop> getAutoSpreadAllLoops() const noexcept;
    /**
     * Sets auto-spread loops. The existing ones are replaced, the other ones are untouched.
     * @param newLoops the new loops.
     */
    void setAutoSpreadLoops(const std::unordered_map<PsgSection, Loop>& newLoops) noexcept;

    /**
     * @return true if the value, at the given index and for the PsgSection, is the result of auto-spread (is thus generated).
     * So returns true if the index is "after" the end of the possible auto-spread loop.
     * @param psgSection the PSG Section.
     * @param cellIndex the cell index. May actually be invalid.
     */
    bool isGeneratedValueAtIndex(PsgSection psgSection, int cellIndex) const noexcept;

    /**
     * Builds a Spread PSG Instrument Cell, that is, a Cell with auto-spread applied. Can then be converted into low-level ("flat") cell.
     * @param cellIndex the index of the cell. If invalid, an empty cell is returned.
     * @return an instance.
     */
    SpreadPsgInstrumentCell buildSpreadPsgInstrumentCell(int cellIndex) const noexcept;

    /** @return true if the sfx is exported. */
    bool isSoundEffectExported() const noexcept;
    /**
     * Sets whether the sfx is exported.
     * @param exported true if exported.
     */
    void setSoundEffectExported(bool exported) noexcept;

    bool operator==(const PsgPart& other) const noexcept;
    bool operator!=(const PsgPart& other) const noexcept;

private:
    /** @return all the sections. */
    static std::vector<PsgSection> getAllSections() noexcept;

    /**
     * Determines a specific value in a Cell, but takes care if the auto-spread feature, so may pick a value from another cell index.
     * @tparam RETURN_TYPE the return type (a link, a period, etc.).
     * @param cellIndex the current cell index.
     * @param originalCell the original cell, for this row.
     * @param cellIndexToCell a map linking a cell index to Cells that we may pick data from. No check is performed!
     * @param autoSpreadLoops all the loops for the auto-spread. May be on or off.
     * @param psgSection the PSG section we are interested in.
     * @param generatedSectionsToFill filled with the current PsgSection if the value is determined as generated.
     * @param functionOnCell the function to call on the cell, to get the value.
     * @return the value to set.
     */
    template<typename RETURN_TYPE>
    static RETURN_TYPE determineValueInCellSlot(const int cellIndex, const PsgInstrumentCell& originalCell,
                                                const std::unordered_map<int, PsgInstrumentCell>& cellIndexToCell,
                                                const std::unordered_map<PsgSection, Loop>& autoSpreadLoops, PsgSection psgSection,
                                                std::set<PsgSection>& generatedSectionsToFill,
                                                const std::function<RETURN_TYPE(const PsgInstrumentCell&)>& functionOnCell) noexcept
    {
        // Gets the loop. If there is none, or current index is before it, the original value is used.
        // We consider that being "in" the loop does not change anything (it is not shown as "generated".
        const auto& loop = autoSpreadLoops.at(psgSection);
        const auto endIndex = loop.getEndIndex();
        if (!loop.isLooping() || (cellIndex <= endIndex)) {
            return functionOnCell(originalCell);
        }

        // We are in the loop!
        const auto startIndex = loop.getStartIndex();
        const auto loopLength = loop.getLength();
        const auto targetCellIndex = ((cellIndex - startIndex) % loopLength) + startIndex;
        jassert(cellIndexToCell.find(targetCellIndex) != cellIndexToCell.cend());       // Trying to reach a cell that we didn't store before! Must NEVER happen!

        // Reads the value from the "other" cell.
        const auto& cellToReadFrom = cellIndexToCell.at(targetCellIndex);
        const auto newValue = functionOnCell(cellToReadFrom);

        // Marks the section as "generated".
        generatedSectionsToFill.insert(psgSection);

        return newValue;
    }

    int speed;                                          // The speed, from 0 (fastest) to 255 (slowest).

    bool instrumentRetrig;                              // True if the Instrument retrigs.
    /**
     * The stored base instrument cells. Should never be empty. The auto-spread does NOT apply.
     * These calls should not be used by player or IE, as there is no auto-spread.
     */
    std::vector<PsgInstrumentCell> cells;

    Loop mainLoop;                                      // The data about the loop.

    std::unordered_map<PsgSection, Loop> sectionToAutoSpreadLoop;     // Links a section to its auto-spread loop.

    bool soundEffectExported;                           // True if the user wants the sfx to be exported. True by default.
};

}   // namespace arkostracker
