#include "SelectionInList.h"

#include "../../utils/SetUtil.h"

namespace arkostracker 
{

SelectionInList::SelectionInList(juce::SparseSet<int> pSelectedIndexes, OptionalInt pLastlySelectedIndex) noexcept :
        selectedIndexes(std::move(pSelectedIndexes)),
        lastlySelectedIndex(pLastlySelectedIndex)
{
}

SelectionInList::SelectionInList(int pSelectedIndex) noexcept :
        selectedIndexes(),
        lastlySelectedIndex(pSelectedIndex)
{
    selectedIndexes.addRange(juce::Range(pSelectedIndex, pSelectedIndex));
}

bool SelectionInList::isEmpty() const noexcept
{
    return selectedIndexes.isEmpty();
}

const juce::SparseSet<int>& SelectionInList::getSelectedIndexes() const noexcept
{
    return selectedIndexes;
}

std::set<int> SelectionInList::getSelectedIndexesAsSet() const noexcept
{
    return SetUtil::sparseSetToSet(selectedIndexes);
}

OptionalInt SelectionInList::getLastlySelectedIndex() const noexcept
{
    // Nothing selected? Then absent.
    if (isEmpty()) {
        return { };
    }

    // If not given, returns the last one.
    return lastlySelectedIndex.isPresent()
           ? lastlySelectedIndex.getValue()
           : selectedIndexes[selectedIndexes.size() - 1];
}

bool SelectionInList::operator==(const SelectionInList& rhs) const
{
    return selectedIndexes == rhs.selectedIndexes &&
           lastlySelectedIndex == rhs.lastlySelectedIndex;
}

bool SelectionInList::operator!=(const SelectionInList& rhs) const
{
    return !(rhs == *this);
}


}   // namespace arkostracker

