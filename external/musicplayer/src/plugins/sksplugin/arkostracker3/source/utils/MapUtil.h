#pragma once

#include <map>
#include <set>

#include <juce_core/juce_core.h>

#include "OptionalValue.h"

namespace arkostracker 
{

/** Utility for maps. */
class MapUtil
{
public:
    /** Prevents instantiation. */
    MapUtil() = delete;

    /**
     * @return the maximum key of the given map. If not found, returns the key default value and asserts.
     * @param inputMap the map.
     */
    template<typename K, typename V>
    static K getMaximumKey(const std::map<K, V>& inputMap) noexcept
    {
        // Sanity check.
        if (inputMap.empty()) {
            jassertfalse;
            return K();
        }

        return inputMap.rbegin()->first;
    }

    /**
     * Finds a key that is not used in the given map. Useful to find "holes" in a map.
     * It may be between keys, or "after".
     * @param inputMap the map.
     * @param firstIndex the start index. Starts from here, and increases.
     * @param lastIndex the last index that can be reached,
     * @return an unused index.
     */
    template<typename K, typename V>
    static K findNextHoleInKeys(const std::map<K, V>& inputMap, const K firstIndex) noexcept
    {
        auto index = firstIndex;
        // There WILL be a hole somewhere!
        while (true) {
            if (inputMap.find(index) == inputMap.cend()) {
                return index;
            }
            ++index;
        }
    }

    /**
     * Indicates whether the keys are contiguous in the given map. Useful to find "holes" in a map.
     * @param inputMap the map.
     * @param firstIndex the start index. Starts from here, and increases.
     * @return absent if everything is fine (no holes), or the index of the FIRST key which is missing. There may be others!
     */
    template<typename K, typename V>
    static OptionalValue<K> areKeysContiguous(const std::map<K, V>& inputMap, const K firstIndex) noexcept
    {
        auto count = inputMap.size();
        // Skips the entries that are below the first index, decreases the count accordingly.
        for (const auto& entry : inputMap) {
            if (entry.first < firstIndex) {
                --count;
            } else {
                break;
            }
        }

        auto index = firstIndex;
        while (count > 0) {
            if (inputMap.find(index) == inputMap.cend()) {
                return index;           // Found a hole.
            }
            ++index;
            --count;
        }

        // Everything is fine.
        return { };
    }

    /**
     * @return the index of the keys which are missing, between all the present keys, if any.
     * @param inputMap the map.
     * @param firstIndex the start index. Starts from here, and increases.
     */
    template<typename K, typename V>
    static std::set<K> findHolesInKeys(const std::map<K, V>& inputMap, const K firstIndex) noexcept
    {
        auto count = inputMap.size();
        // Important! Skips the entries that are below the first index, decreases the count accordingly. Else, the algorithm will never end.
        for (const auto& entry : inputMap) {
            if (entry.first < firstIndex) {
                --count;
            } else {
                break;
            }
        }

        std::set<K> holeIndexes;

        auto index = firstIndex;
        while (count > 0) {
            if (inputMap.find(index) == inputMap.cend()) {
                holeIndexes.insert(index);           // Found a hole.
            } else {
                --count;
            }
            ++index;
        }

        return holeIndexes;
    }

    /**
     * Fills a map which has "holes", with a default object. This uses a values inside a unique_ptr.
     * @param firstIndex the first index where a hole can be. Keys below this value are NOT removed.
     * @param indexToItem the map linking an index to an item.
     * @param createNewItem a lambda to create the new item.
     */
    template<typename ITEM>
    static void fillHolesInMapUniquePtr(int firstIndex, std::map<int, std::unique_ptr<ITEM>>& indexToItem,
                                        const std::function<std::unique_ptr<ITEM>()>& createNewItem) noexcept
    {
        // Any hole?
        const auto holeIndexes = findHolesInKeys(indexToItem, firstIndex);
        for (auto holeIndex : holeIndexes) {
            auto item = createNewItem();
            indexToItem.insert(std::make_pair(holeIndex, std::move(item)));
        }
    }

    /**
     * Fills a map which has "holes", with a default object.
     * @param firstIndex the first index where a hole can be. Keys below this value are NOT removed.
     * @param indexToItem the map linking an index to an item.
     * @param createNewItem a lambda to create the new item.
     */
    template<typename ITEM>
    static void fillHolesInMap(int firstIndex, std::map<int, ITEM>& indexToItem, const std::function<ITEM()>& createNewItem) noexcept
    {
        // Any hole?
        const auto holeIndexes = findHolesInKeys(indexToItem, firstIndex);
        for (auto holeIndex : holeIndexes) {
            const auto& item = createNewItem();
            indexToItem.insert(std::make_pair(holeIndex, item));
        }
    }

    /**
     * Creates a vector of items from a map of these items. Holes are filled with default-constructed items.
     * Once "count" is reached, stops.
     * @param startIndex the index at which the map is started being browsed.
     * @param count how many items the final collection will have.
     * @param indexToItem the map linking an index to an item.
     * @return the vector.
     */
    template<typename ITEM, typename MAP>
    static std::vector<ITEM> createVectorFromMap(const int startIndex, const int count, const MAP& indexToItem) noexcept
    {
        std::vector<ITEM> items;

        auto index = startIndex;
        while (static_cast<int>(items.size()) < count) {            // Stupid warning...
            auto iterator = indexToItem.find(index);
            if (iterator == indexToItem.cend()) {
                // Not found. Uses a default item.
                items.emplace_back();
            } else {
                items.push_back(iterator->second);
            }
            ++index;
        }

        return items;
    }

    /**
     * Creates a vector from each entry of a given map. The keys are ignored.
     * @param indexToItem the map.
     * @return a vector.
     */
    template<typename ITEM, typename MAP>
    static std::vector<ITEM> createVector(const MAP& map) noexcept
    {
        std::vector<ITEM> items;

        for (const auto&[key, value] : map) {
            items.push_back(value);
        }

        return items;
    }

    /**
     * Deletes items from a map from a predicate.
     * Stolen from https://stackoverflow.com/a/16597048.
     * @param map the map.
     * @param predicate the predicate.
     */
    template<typename MAP, typename PREDICATE>
    static void eraseIf(MAP& map, const PREDICATE& predicate)
    {
        for (auto it = map.begin(); it != map.end();) {
            if (predicate(*it)) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
    }
};

}   // namespace arkostracker

