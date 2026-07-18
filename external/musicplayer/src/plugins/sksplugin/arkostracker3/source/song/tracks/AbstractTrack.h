#pragma once

#include <map>

#include <juce_core/juce_core.h>

#include "../../utils/Counter.h"
#include "TrackConstants.h"

namespace arkostracker 
{

/**
 * This is a template for Tracks, which can be for example "Track" (or Music Track), Speed Track or Event Track.
 *
 * A Track consists in Cells. The type of the Cell is parameterized by the template.
 *
 * A track may have no cells. However, if the client asks for a cell index that doesn't exist,
 * an empty cell is returned nonetheless.
 *
 * A Track can hold at most 128 Cells. Getting or setting beyond Cells has no action or return empty Cells.
 *
 * Each Cell must have a default constructor, used in case of an out-of-bound Cell is read, or an empty Cell is read.
 */
template<typename ElementType>
class AbstractTrack         // TODO TU this.
{
public:
    /** Constructor of an empty track. */
    AbstractTrack() noexcept :
            name(),
            color(),
            indexToCell(),
            modificationCount(increaseModificationCounterAndGet()),
            emptyCell(),
            readOnly()
    {
    }

    /** Constructor of an empty track with a name and ARGB color. */
    explicit AbstractTrack(juce::String pName, const OptionalValue<juce::uint32> pARGBColor = { }) noexcept :
            name(std::move(pName)),
            color(pARGBColor),
            indexToCell(),
            modificationCount(increaseModificationCounterAndGet()),
            emptyCell(),
            readOnly()
    {
    }

    /**
     * Sets the name.
     * @param newName the new name of the Track.
     */
    void setName(const juce::String& newName) noexcept
    {
        name = newName;
        increaseModificationCounter();
    }

    /** @returns the name of the track. */
    juce::String getName() const noexcept
    {
        return name;
    }

    /** @returns the possible color, ARGB. */
    OptionalValue<juce::uint32> getColor() const noexcept
    {
        return color;
    }

    /**
     * Sets the color.
     * @param newARGBColor the new color.
     */
    void setColor(const OptionalValue<juce::uint32> newARGBColor) noexcept
    {
        color = newARGBColor;
        increaseModificationCounter();
    }

    /** @return true if the name not blank. */
    bool isNamed() const noexcept
    {
        return name.trim().isNotEmpty();
    }

    /** @return true if read-only. */
    bool isReadOnly() const noexcept
    {
        return readOnly;
    }

    /**
     * Set the read-only flag.
     * @param newReadOnly true if read-only.
     */
    void setReadOnly(const bool newReadOnly) noexcept
    {
        readOnly = newReadOnly;
        increaseModificationCounter();
    }

    /** Toggles the read-only. */
    void toggleReadOnly() noexcept
    {
        setReadOnly(!readOnly);
    }

    /** Clears all the cells (the name does not change), regardless of read-only. */
    void clearCells() noexcept
    {
        if (readOnly) {
            jassertfalse;
            return;
        }

        indexToCell.clear();
        increaseModificationCounter();
    }

    /**
     * @returns a Cell from its index. If out of the boundaries, an empty Cell is returned.
     * @param cellIndex the cell index.
     */
    ElementType getCell(const int cellIndex) const noexcept
    {
        return getCellRefConst(cellIndex);      // Copies the result.
    }

    /**
     * @returns a reference to a Cell from its index. If out of the boundaries, an empty Cell is returned.
     * The Track should be locked before using this method!
     * @param cellIndex the cell index.
     */
    const ElementType& getCellRefConst(const int cellIndex) const noexcept
    {
        // Outside boundaries? Returns an empty Cell.
        if ((cellIndex < 0) || (cellIndex > TrackConstants::lastPossibleIndex)) {
            return emptyCell;
        }

        auto iterator = indexToCell.find(cellIndex);
        if (iterator == indexToCell.cend()) {
            return emptyCell;           // Not in the map, so empty Cell.
        }

        return iterator->second;
    }

    /**
     * Sets a cell. Out of boundaries cells are ignored.
     * @param cellIndex the index of the cell. May be invalid.
     * @param cell the cell to set.
     */
    void setCell(const int cellIndex, const ElementType& cell) noexcept
    {
        if (readOnly) {
            jassertfalse;
            return;
        }

        // In case of an out-of-boundary Cell, do nothing.
        if (!isCellIndexValid(cellIndex)) {
            return;
        }

        setCellRaw(cellIndex, cell);
    }

    /** @return the size. It always returns the maximum capacity. */
    static int getSize() noexcept
    {
        return TrackConstants::maximumCapacity;     // Getting/setting data is always possible within this capacity.
    }

    /** @return whether the Track is empty or not. */
    bool isEmpty() const noexcept
    {
        return indexToCell.empty();
    }

    /**
     * @return true if the Cell is empty. Out of bound cells are considered empty.
     * @param cellIndex the cell index. May be out of bounds.
     */
    bool isCellEmpty(const int cellIndex) const noexcept
    {
        // Outside boundaries?
        if ((cellIndex < 0) || (cellIndex > TrackConstants::lastPossibleIndex)) {
            return true;
        }

        auto iterator = indexToCell.find(cellIndex);
        return (iterator == indexToCell.cend());    // Not in the map, so empty Cell.
    }

    /**
     * Finds the minimum height (that is, the latest non-empty cell).
     * @return the minimum height (>=0, 0 if the Track is empty).
     */
    int findMinimumHeight() const noexcept
    {
        if (isEmpty()) {
            return 0;
        }

        return (indexToCell.crbegin()->first) + 1;        // Only non-empty Cells are written, so this is simple and safe.
    }

    /** Creates a new Track based on this one, but keeps only the items below the given height (>=0). */
    AbstractTrack reducedTo(int height) const {
        jassert(height >= 0);

        // Duplicate the Cells, keeping only those within the height.
        decltype(indexToCell) newCells;
        for (auto&[index, cell] : indexToCell) {
            if (index < height) {
                newCells.emplace(index, cell);
            } else {
                break;
            }
        }

        // Creates a new Track.
        auto newTrack = AbstractTrack();
        newTrack.indexToCell = newCells;
        newTrack.name = name;
        newTrack.readOnly = readOnly;

        return newTrack;
    }

    /**
     * @return true if the current Track content is contained in the other Track. This means their Cells must be equals up to
     * the height of the current Track.
     * @param height the height the first Track must be compare up to.
     * @param other the other Track.
     */
    bool isContainedIn(const int height, const AbstractTrack other) const {
        // Not the brightest algorithm, but simple enough.
        for (auto cellIndex = 0; cellIndex < height; ++cellIndex) {
            if (getCellRefConst(cellIndex) != other.getCellRefConst(cellIndex)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @return true if the modification counter of the Track and the given one are the same.
     * @param otherTrack the other track to compare the counter to.
     */
    bool isSameModificationCounter(const AbstractTrack& otherTrack) const noexcept {
        return (modificationCount == otherTrack.modificationCount);
    }

    /** @return the index and items. Only non-empty are returned. */
    std::vector<std::pair<int, ElementType>> getNonEmptyItems() const noexcept
    {
        std::vector<std::pair<int, ElementType>> results;

        for (const auto& entry : indexToCell) {
            const auto index = entry.first;
            const auto item = entry.second;

            jassert(!item.isEmpty());        // Only non-empty items should be encoded!

            results.emplace_back(index, item);
        }

        return results;
    }

    /**
     * @return the cell present at the given index, but if empty, will go back to the first before it can find.
     * If none is found, an empty cell is returned.
     * @param maximumIndex the maximum index (typically the pattern height - 1).
     */
    ElementType getLatestCell(const int maximumIndex) const noexcept
    {
        jassert((maximumIndex >= 0) && (maximumIndex <= TrackConstants::lastPossibleIndex));

        if (isEmpty()) {
            return emptyCell;
        }

        // Starts the browsing from the end. I tried to use map::lower_bound, but it was actually more complicated...
        for (auto iterator = indexToCell.rbegin(); iterator != indexToCell.rend(); ++iterator) {
            const auto readIndex = iterator->first;
            if (readIndex > maximumIndex) {         // Value too high, discarded.
                continue;
            }
            jassert(readIndex <= maximumIndex);    // Just to sure the map is well sorted...
            return iterator->second;                        // Found!
        }

        // Not found.
        return emptyCell;
    }

    /**
     * Inserts an empty cell in a Track.
     * @param cellIndex the cell index. May be out of bounds, in which case nothing happens and an empty Cell is returned.
     * @param elementToInsert the cell to insert. Can be an empty item.
     * @return the Cell that has gone beyond the track capacity.
     */
    ElementType insertCellAt(const int cellIndex, const ElementType& elementToInsert) noexcept
    {
        return insertOrRemoveCellAt(cellIndex, cellIndex, elementToInsert, TrackConstants::lastPossibleIndex, 1);
    }

    /**
     * Removes a Cell, shifting "up" the other Cells after, and writing a Cell at the end.
     * @param cellIndex the cell where the removal is performed.
     * @param lastCellToWrite the cell to write at the end.
     * @return the removed Cell.
     */
    ElementType removeCellAt(const int cellIndex, const ElementType& lastCellToWrite) noexcept
    {
        return insertOrRemoveCellAt(cellIndex, TrackConstants::lastPossibleIndex, lastCellToWrite, cellIndex, -1);
    }

    bool operator==(const AbstractTrack& other) const noexcept
    {
        return (name == other.name)
            && (indexToCell == other.indexToCell)
            && (readOnly == other.readOnly)
        ;
    }

    bool operator!=(const AbstractTrack& other) const noexcept
    {
        return !(other == *this);
    }

protected:

    /**
     * Indicates whether the given cell index is valid (positive and < maximum capacity).
     * @param cellIndex the cell index. May be invalid.
     * @return true if the given index is valid.
     */
    static bool isCellIndexValid(const int cellIndex) noexcept
    {
        return ((cellIndex >= 0) && (cellIndex < TrackConstants::maximumCapacity));
    }

private:
    /**
     * Generic code to insert or remove an empty cell in a Track, shifting the other Cells.
     * @param cellIndex the cell index where to insert/remove. May be out of bounds, in which case nothing happens and an empty Cell is returned.
     * @param indexWhereToWriteCell where to write the given cell.
     * @param cellToWrite the cell to write. For an insertion, this will be an empty cell.
     * @param indexOfCellToReturn the index of the Cell to return (BEFORE the shift is performed!).
     * @param shiftDirection positive or negative.
     * @return the Cell that has gone beyond the track capacity.
     */
    ElementType insertOrRemoveCellAt(const int cellIndex, const int indexWhereToWriteCell, const ElementType& cellToWrite, const int indexOfCellToReturn,
                                     const int shiftDirection) noexcept
    {
        if (readOnly) {
            jassertfalse;
            return emptyCell;
        }

        if ((cellIndex < 0) || (cellIndex >= TrackConstants::maximumCapacity)) {
            jassertfalse;           // Shouldn't happen...
            return emptyCell;
        }

        // Gets the item that is going to be removed.
        ElementType itemGoingToBeRemoved = emptyCell;
        if (auto iterator = indexToCell.find(indexOfCellToReturn); iterator != indexToCell.cend()) {
            itemGoingToBeRemoved = iterator->second;
        }

        // Much simpler to create a new Map.
        decltype(indexToCell) newIndexToCell;
        for (const auto&[index, item] : indexToCell) {
            // If before the insertion point, don't modify the item index.
            if (index < cellIndex) {
                newIndexToCell[index] = item;
            } else {
                // After the insertion point. Discards if out of bounds.
                const auto newIndex = index + shiftDirection;
                if ((newIndex >= cellIndex) && (newIndex <= TrackConstants::lastPossibleIndex)) {
                    newIndexToCell[newIndex] = item;
                }
            }
        }

        indexToCell = newIndexToCell;

        // Writes the new Cell. This takes care of: checking whether the cell is empty (which should not be encoded), and increasing the modification counter.
        setCellRaw(indexWhereToWriteCell, cellToWrite);

        return itemGoingToBeRemoved;
    }
    /**
     * Sets a cell. Must be within valid bounds (not tested).
     * If the cell is empty, the entry is deleted.
     * The modification counter is increased.
     * @param cellIndex the index of the cell. May be invalid.
     * @param cell the cell to set.
     */
    void setCellRaw(const int cellIndex, const ElementType& cell) noexcept
    {
        if (readOnly) {
            jassertfalse;
            return;
        }

        if (cell.isEmpty()) {
            indexToCell.erase(cellIndex);       // Empty Cells must not be encoded.
        } else {
            indexToCell[cellIndex] = cell;
        }

        increaseModificationCounter();
    }

    /** Increases the internal modification counter, accordingly to a global one. */
    void increaseModificationCounter() noexcept
    {
        modificationCount = Counter::getNextCounter();
    }

    /** Increases the internal modification counter, and returns it. */
    unsigned int increaseModificationCounterAndGet() noexcept
    {
        increaseModificationCounter();
        return modificationCount;
    }

    juce::String name;                                          // The name of the track.
    OptionalValue<juce::uint32> color;                          // The possible color, ARGB.
    std::map<int, ElementType> indexToCell;                     // The cells. Only non-empty Cells should be encoded. Ordered to ease parsing.

    unsigned int modificationCount;                 // A simple integer that increases each time the Track is modified (even the name). Only to know if a display is obsolete.
    ElementType emptyCell;                          // Used when an out-of-boundaries is used.
    bool readOnly;                                  // True if it cannot be changed (name not included).
};

}   // namespace arkostracker
