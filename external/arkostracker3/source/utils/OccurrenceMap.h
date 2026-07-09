#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace arkostracker 
{

/**
 * Class that holds a map, which key is anything, and the value the occurrence it has.
 */
template<typename ITEM>
class OccurrenceMap
{
public:
    /** Constructor. */
    OccurrenceMap() noexcept :
        itemToOccurrence()
    {
    }

    /**
     * Adds an item. If it already exists, it occurrence count increases.
     * @param item the item to add.
     */
    void addItem(const ITEM& item) noexcept
    {
        // Does the item already exist?
        auto iterator = itemToOccurrence.find(item);
        if (iterator == itemToOccurrence.end()) {
            // New instrument, one iteration.
            itemToOccurrence.insert(std::make_pair(item, 1));
        } else {
            // Instrument already there. Increases the iteration count.
            auto& iterationCount = iterator->second;
            ++iterationCount;
        }
    }

    /**
     * @return how many different items there are so far.
     */
    size_t getSize() noexcept
    {
        return itemToOccurrence.size();
    }

    /** @return a copy of the the map linking items to how many times they were used. */
    std::unordered_map<int, unsigned int> getItemOccurrenceMap() const noexcept
    {
        return itemToOccurrence;
    }

    /** @return the most used indexes, from most to least used. It is built on the fly. */
    std::vector<std::pair<int, unsigned int>> generateSortedMostUsedItemsAndOccurrence() const noexcept
    {
        // Creates a copy of the map, and sorts them from their occurrence.
        std::vector<std::pair<int, unsigned int>> map(itemToOccurrence.begin(), itemToOccurrence.end());

        std::sort(map.begin(), map.end(), [](const std::pair<ITEM, unsigned int>& left, const std::pair<ITEM, unsigned int>& right) {
            return left.second > right.second;
        });

        return map;
    }

private:
    std::unordered_map<ITEM, unsigned int> itemToOccurrence;			// Map linking items to how many times they were used.
};

}   // namespace arkostracker

