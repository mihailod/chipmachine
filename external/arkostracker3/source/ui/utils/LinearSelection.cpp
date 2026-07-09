#include "LinearSelection.h"

#include "../../utils/NumberUtil.h"
#include "../../utils/SetUtil.h"

namespace arkostracker 
{

LinearSelection::LinearSelection() noexcept :
        selectedItems(),
        firstSelectedPosition(0),
        lastSelectedPosition(0)
{
}

bool LinearSelection::isEmpty() const noexcept
{
    return selectedItems.empty();
}

void LinearSelection::selectOnly(const int index) noexcept
{
    selectedItems.clear();
    selectedItems.insert(index);
    firstSelectedPosition = index;
    lastSelectedPosition = index;
}

void LinearSelection::toggle(const int index) noexcept
{
    if (selectedItems.find(index) == selectedItems.cend()) {
        selectedItems.insert(index);
    } else {
        selectedItems.erase(index);
    }

    // If we only have one item now, considers it to be the new "first", else use the toggled one.
    // If there is no one left, we don't care, the first/last index are relevant only if the selection is not empty.
    auto newIndex = index;
    if (selectedItems.size() == 1) {
        newIndex = *selectedItems.cbegin();
    }

    firstSelectedPosition = newIndex;
    lastSelectedPosition = newIndex;
}

void LinearSelection::growTo(const int endIndex) noexcept
{
    if (isEmpty()) {
        return;
    }

    selectFromTo(firstSelectedPosition, endIndex);
}

bool LinearSelection::isSingle() const noexcept
{
    return (selectedItems.size() == 1);
}

bool LinearSelection::isSelected(const int index) const noexcept
{
    return (selectedItems.find(index) != selectedItems.cend());
}

void LinearSelection::clear() noexcept
{
    selectedItems.clear();
}

const std::set<int>& LinearSelection::getSelectedItems() const noexcept
{
    return selectedItems;
}

int LinearSelection::getLowest() const noexcept
{
    return SetUtil::min(selectedItems);
}

int LinearSelection::getHighest() const noexcept
{
    return SetUtil::max(selectedItems);
}

int LinearSelection::getLastSelectedItem() const noexcept
{
    if (isEmpty()) {
        return 0;
    }
    return lastSelectedPosition;
}

bool LinearSelection::growOf(const int count, const int minimum, const int maximum) noexcept
{
    if ((count == 0) || isEmpty()) {
        return false;
    }

    const auto newLastSelectedPosition = NumberUtil::correctNumber(lastSelectedPosition + count, minimum, maximum);
    selectFromTo(firstSelectedPosition, newLastSelectedPosition);

    return true;
}

void LinearSelection::selectFromTo(const int from, const int to) noexcept
{
    selectedItems.clear();

    if (to >= from) {
        // Larger index.
        for (auto index = from; index <= to; ++index) {
            selectedItems.insert(index);
        }
    } else {
        // Smaller index.
        for (auto index = from; index >= to; --index) {
            selectedItems.insert(index);
        }
    }
    firstSelectedPosition = from;
    lastSelectedPosition = to;
}

bool LinearSelection::correct(const int minimum, const int maximum) noexcept
{
    if (isEmpty()) {
        return false;
    }
    jassert(maximum >= minimum);

    // Remove_if doesn't work with sets... C++...
    std::set<int> newSelectedItems;
    std::copy_if(selectedItems.begin(), selectedItems.end(),
                 std::inserter(newSelectedItems, newSelectedItems.end()),       // https://stackoverflow.com/questions/24386859/filtering-a-stdset-based-on-specific-data
                 [&] (const int i) { return ((i >= minimum) && (i <= maximum)); });

    if (newSelectedItems.size() == selectedItems.size()) {
        return false;
    }
    selectedItems = newSelectedItems;

    if (selectedItems.empty()) {
        // Nothing anymore. Tries to select the last possible item.
        selectedItems.insert(maximum);
    }

    // Are the first/last positions still inside the new selection? If not, corrects them.
    if (selectedItems.find(firstSelectedPosition) == selectedItems.cend()) {
        // Takes the first.
        firstSelectedPosition = *selectedItems.cbegin();
    }
    if (selectedItems.find(lastSelectedPosition) == selectedItems.cend()) {
        // Takes the last.
        lastSelectedPosition = *selectedItems.crbegin();
    }

    return true;
}

bool LinearSelection::contains(const int index) const noexcept
{
    return (selectedItems.find(index) != selectedItems.cend());
}

int LinearSelection::getSelectedCount() const noexcept
{
    return static_cast<int>(selectedItems.size());
}

void LinearSelection::fillFrom(const std::set<int>& itemsToSelect) noexcept
{
    selectedItems = itemsToSelect;
    firstSelectedPosition = getLowest();
    lastSelectedPosition = getHighest();
}

void LinearSelection::fillFrom(const std::unordered_set<int>& itemsToSelect) noexcept
{
    auto set = SetUtil::toOrderedSet(itemsToSelect);
    fillFrom(set);
}

void LinearSelection::fillFrom(const int itemToSelect) noexcept
{
    fillFrom(std::set { itemToSelect });
}

}   // namespace arkostracker

