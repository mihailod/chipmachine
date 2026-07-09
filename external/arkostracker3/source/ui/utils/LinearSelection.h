#pragma once

#include <set>                  // Using an ordered set is important.
#include <unordered_set>

namespace arkostracker 
{

/** An object to manage selection in a "linear" way (only one dimension). */
// FIXME Probably a problem when selecting two separate item, and toggle out the first one.
// TODO I think it should be overhauled partially. In software, the cursor becomes the selection. The first would be the lowest index, the last the highest.
// When going left, use the first index, when going right, use the last index.
class LinearSelection           // TODO TU this.
{
public:
    /** Constructor. */
    LinearSelection() noexcept;

    /** @return true if there is no selection. */
    bool isEmpty() const noexcept;

    /**
     * @return true if the selection contains the given index.
     * @param index the index.
     */
    bool contains(int index) const noexcept;

    /** @return true if the index is selected. */
    bool isSelected(int index) const noexcept;

    /** Clears the selection. */
    void clear() noexcept;

    /** @return the selected items, in order. */
    const std::set<int>& getSelectedItems() const noexcept;

    /** Clears and fills the selection from the given items. May be empty. */
    void fillFrom(const std::set<int>& itemsToSelect) noexcept;
    /** Clears and fills the selection from the given items (unordered as a convenience). May be empty. */
    void fillFrom(const std::unordered_set<int>& itemsToSelect) noexcept;
    /** Clears and fills the selection from the given item. */
    void fillFrom(int itemToSelect) noexcept;

    /** @return the lastly selected item. If not doable, returns 0. */
    int getLastSelectedItem() const noexcept;
    /** @return the lowest item. If empty, returns 0. */
    int getLowest() const noexcept;
    /** @return the highest item. If empty, returns 0. */
    int getHighest() const noexcept;
    /** @return true if there is only one item. */
    bool isSingle() const noexcept;

    /** Marks an index as "selected", all the others are unselected. */
    void selectOnly(int index) noexcept;

    /**
     * Toggles the selection of an index.
     * @param index the index to toggle.
     */
    void toggle(int index) noexcept;

    /**
     * Grows the selection, from the first selected position to the given one.
     * If empty, nothing happens.
     */
    void growTo(int endIndex) noexcept;

    /**
     * Makes the selection grows.
     * @param count how many items to grow of. May be negative.
     * @param minimum the minimum value that can be reached.
     * @param maximum the maximum value that can be reached.
     * @return true if something has changed, false if not.
     */
    bool growOf(int count, int minimum, int maximum) noexcept;

    /**
     * Corrects the current selection. If empty, nothing is done.
     * @param minimum the minimum value that can be reached.
     * @param maximum the maximum value that can be reached.
     * @return true if something has changed, false if not.
     */
    bool correct(int minimum, int maximum) noexcept;

    /** @return how many items are selected. */
    int getSelectedCount() const noexcept;

private:
    /**
     * Makes the selection be from an item to another. In any order!
     * This will updates all internal data.
     * @param from the start item.
     * @param to the end item.
     */
    void selectFromTo(int from, int to) noexcept;

    std::set<int> selectedItems;                            // The Items that are selected. The ordering is important.
    int firstSelectedPosition;                              // The item that has been selected first. Only relevant if selectedPositions is not empty.
    int lastSelectedPosition;                               // The item that has been lastly first. Only relevant if selectedPositions is not empty.
};

}   // namespace arkostracker

