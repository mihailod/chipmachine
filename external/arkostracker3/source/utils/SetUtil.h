#pragma once

#include <algorithm>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Utility methods for Sets. */
class SetUtil                   // TODO TU this.
{
public:
    /** Prevents instantiation. */
    SetUtil() = delete;

    /**
     * @return the minimum value this Set contains. If none is found, a default value is returned.
     * @param set the Set.
     */
    template<typename VALUE>
    static VALUE min(const std::set<VALUE>& set) noexcept {
        if (set.empty()) {
            return VALUE();
        }

        return *std::min_element(set.cbegin(), set.cend());
    }

    /**
     * @return the maximum value this Set contains. If none is found, a default value is returned.
     * @param set the Set.
     */
    template<typename VALUE>
    static VALUE max(const std::set<VALUE>& set) noexcept {
        if (set.empty()) {
            return VALUE();
        }

        return *std::max_element(set.cbegin(), set.cend());
    }

    /**
     * Creates a Set with linear values.
     * @param firstIndex the first index.
     * @param after if true, 1 is added to the first index. This is a convenience parameter, useful in many cases.
     * @param count how many items there are. >=0.
     * @return the Set.
     */
    template<typename VALUE>
    static std::set<VALUE> createLinearItems(const int firstIndex, const bool after, const int count) noexcept {
        jassert(count >= 0);

        auto items = std::set<int>();
        const auto destinationIndex = firstIndex + (after ? 1 : 0);
        for (auto i = 0; i < count; ++i) {
            items.insert(destinationIndex + i);
        }

        return items;
    }

    /**
     * @return an ordered Set from the given unordered one.
     * @param originalUnorderedSet the original set. May be empty.
     */
    template<typename VALUE>
    static std::set<VALUE> toOrderedSet(const std::unordered_set<VALUE>& originalUnorderedSet) noexcept {
        return std::set<int>(originalUnorderedSet.cbegin(), originalUnorderedSet.cend());
    }

    /**
     * Puts the content of a JUCE SparseSet into a Set.
     * @tparam TYPE the type of the items.
     * @param inputSparseSet the input SparseSet.
     * @return the Set, with the SparseSet values.
     */
    template<typename TYPE>
    static std::set<TYPE> sparseSetToSet(const juce::SparseSet<TYPE>& inputSparseSet) noexcept
    {
        std::set<int> set;
        for (int i = 0, size = inputSparseSet.size(); i < size; ++i) {
            set.insert(inputSparseSet[i]);
        }

        return set;
    }

    /**
     * Creates a Set from the given Collection. (note: unable to generalize the collections :( ).
     * @tparam TYPE the type of the items.
     * @param inputCollection the input collection.
     * @return the Set.
     */
    template<class TYPE>
    static std::set<TYPE> toSet(const std::vector<TYPE>& inputCollection) noexcept
    {
        std::set<TYPE> outputCollection;
        for (const auto& item : inputCollection) {
            outputCollection.insert(item);
        }

        return outputCollection;
    }

    /**
     * Creates an Unordered Set from the given Collection.
     * @tparam TYPE the type of the items.
     * @param inputCollection the input collection.
     * @return the Set.
     */
    template<class TYPE>
    static std::unordered_set<TYPE> toUnorderedSet(const std::vector<TYPE>& inputCollection) noexcept
    {
        std::unordered_set<TYPE> outputCollection;
        for (const auto& item : inputCollection) {
            outputCollection.insert(item);
        }

        return outputCollection;
    }

    /**
     * Adds an item to a map of vector. If no vector exists, it is created.
     * @tparam KEY the type of the key/index.
     * @tparam ITEM the type of the items.
     * @param indexToItems map linking an index to the items.
     * @param index the key index where to add the item.
     * @param itemToAdd the item to add.
     */
    template<typename KEY, typename ITEM>
    static void addItemToMapOfVectors(std::unordered_map<KEY, std::vector<ITEM>>& indexToItems, KEY index, const ITEM& itemToAdd) noexcept
    {
        // Does the entry in the map exist? If not, creates it.
        if (indexToItems.find(index) == indexToItems.cend()) {
            indexToItems.insert(std::make_pair(index, std::vector<ITEM>()));
        }

        // Adds the entry.
        indexToItems.at(index).push_back(itemToAdd);
    }

    /**
     * @return true if the Set contains the item. It must NOT be used for anything else than a Set, else
     * the full tree will be iterated.
     * @param inputSet the set to search.
     * @param item the item to search for.
     */
    template<typename ITEM, typename SET>
    static bool contains(const SET& inputSet, const ITEM& item) noexcept
    {
        return (inputSet.count(item) > 0);
    }

    /**
     * @return true if the Set contains at least one of the item.
     * @param inputSet the set to search.
     * @param itemSet the items to search for.
     */
    template<typename SET>
    static bool containsOneOf(const SET& inputSet, const SET& itemSet) noexcept
    {
        for (const auto& item : itemSet) {
            if (inputSet.find(item) != inputSet.cend()) {
                return true;
            }
        }

        return false;       // Not found.
    }

    /** @return a vector of items from a set of items. */
    template<typename ITEM>
    static std::vector<ITEM> setToVector(const std::set<ITEM>& inputSet) noexcept
    {
        return { inputSet.cbegin(), inputSet.cend() };
    }
};

}   // namespace arkostracker
