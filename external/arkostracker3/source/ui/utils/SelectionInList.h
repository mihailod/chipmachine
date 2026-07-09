#pragma once

#include <set>

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Selection indexes, and the lastly selected rank. */
class SelectionInList
{
public:
    /**
     * Constructor.
     * @param selectedIndexes the selected indexes. May be empty.
     * @param lastlySelectedIndex the lastly selected index. May be absent, but if that's the case, getting it will be the last of the selected indexes.
     */
    SelectionInList(juce::SparseSet<int> selectedIndexes, OptionalInt lastlySelectedIndex) noexcept;

    /**
     * Constructor, with only one selected index.
     * @param selectedIndex the selected index.
     */
    explicit SelectionInList(int selectedIndex) noexcept;

    /** True if nothing was selected. */
    bool isEmpty() const noexcept;

    /** @return the selected indexes. May be empty. */
    const juce::SparseSet<int>& getSelectedIndexes() const noexcept;
    /** @return the selected indexes, as a set. May be empty. */
    std::set<int> getSelectedIndexesAsSet() const noexcept;

    /** @return the lastly selected index. Only absent if there was nothing selected. */
    OptionalInt getLastlySelectedIndex() const noexcept;

    bool operator==(const SelectionInList& rhs) const;
    bool operator!=(const SelectionInList& rhs) const;

private:
    juce::SparseSet<int> selectedIndexes;
    OptionalInt lastlySelectedIndex;
};


}   // namespace arkostracker

