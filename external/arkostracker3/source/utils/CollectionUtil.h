#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <juce_core/juce_core.h>

#include "OptionalValue.h"

namespace arkostracker 
{

/** Utility  methods about Collections. */
class CollectionUtil
{
public:
    /** Prevents instantiation. */
    CollectionUtil() = delete;

    /**
     * @return a collection of pointer of items, from a collection of unique pointer to the items.
     * @tparam INPUT_ITEM the type of the collection items.
     * @tparam OUTPUT_ITEM the type of the output collection items. May be the same as the input, or not, useless for polymorphism.
     * @param collection the input collection.
     */
    template<typename INPUT_ITEM, typename OUTPUT_ITEM>
    static std::vector<OUTPUT_ITEM*> toPointerCollection(const std::vector<std::unique_ptr<INPUT_ITEM>>& collection) noexcept
    {
        std::vector<OUTPUT_ITEM*> result;
        result.reserve(collection.size());

        for (auto& item : collection) {
            result.push_back(item.get());
        }

        return result;
    }

    /**
     * @return a vector from the given map.
     * @param map the input map.
     */
    template<typename MAP, typename KEY>
    static std::vector<KEY> mapToVector(const MAP& map) noexcept
    {
        std::vector<KEY> outputVector;
        for (auto iterator = map.cbegin(); iterator != map.cend(); ++iterator) {
            outputVector.emplace_back(iterator->first);
        }

        return outputVector;
    }

    /**
     * Append a collection to the end of another.
     * @param collectionToAppend the map to append to the other one.
     * @param collectionToFill the collection to fill with the other.
     */
    template<typename COLLECTION_TO_APPEND, typename COLLECTION_TO_FILL>
    static void append(const COLLECTION_TO_APPEND& collectionToAppend, COLLECTION_TO_FILL& collectionToFill) noexcept
    {
        // Not as efficient as std:insert, but works in more cases.
        std::copy(collectionToAppend.cbegin(), collectionToAppend.cend(), std::back_inserter(collectionToFill));
    }

    /**
     * Finds a value in a map, returns a default value if not found.
     * @tparam KEY the key type.
     * @tparam ITEM the value type.
     * @param map the map to search in.
     * @param keyToSearch the key to look for.
     * @param defaultValue the value used if not found.
     * @param assertIfNotFound raised an assertion if not found.
     * @return the value, or the default if not found.
     */
    template<typename KEY, typename ITEM>
    static ITEM findAndGet(const std::unordered_map<KEY, ITEM>& map, const KEY& keyToSearch, const ITEM& defaultValue, bool assertIfNotFound = false) noexcept
    {
        auto iterator = map.find(keyToSearch);
        if (iterator != map.cend()) {
            return iterator->second;
        }

        // Not found.
        if (assertIfNotFound) {
            jassertfalse;
        }
        return defaultValue;
    }

    /**
     * @return true if the given collection contains the value.
     * @tparam COLLECTION the collection type.
     * @tparam ITEM the value type.
     * @param collection the collection to search in.
     * @param item the item to look for.
     */
    template<typename COLLECTION, typename ITEM>
    static bool contains(const COLLECTION& collection, const ITEM& item) noexcept
    {
        return std::find(collection.cbegin(), collection.cend(), item) != collection.cend();
    }

    /**
     * Finds a Key from the given value, in a map. This is not efficient: the whole collection is browsed till the item is found.
     * @tparam KEY the key type.
     * @tparam VALUE the value type.
     * @param map the map to search in.
     * @param valueToSearch the value to search.
     * @return the key, or empty if not found.
     */
    template<typename KEY, typename VALUE>
    static OptionalValue<KEY> findKeyFromValue(const std::unordered_map<KEY, VALUE>& map, const VALUE& valueToSearch) noexcept
    {
        for (const auto& entry : map) {
            if (entry.second == valueToSearch) {
                return entry.first;
            }
        }

        return OptionalValue<KEY>();
    }

    /**
     * Copies a vector of unique pointer of items.
     * @tparam ITEM the type of the items.
     * @param inputCollection the input collection.
     * @return the copy.
     */
    template<typename ITEM>
    static std::vector<std::unique_ptr<ITEM>> copyVectorOfUniquePtr(const std::vector<std::unique_ptr<ITEM>>& inputCollection) noexcept
    {
        std::vector<std::unique_ptr<ITEM>> outputCollection;
        outputCollection.reserve(inputCollection.size());

        for (const auto& item : inputCollection) {
            outputCollection.push_back(std::make_unique<ITEM>(*item));
        }

        return outputCollection;
    }

    /**
     * Finds the index of an item in a collection.
     * @return the index of the given item to find, inside the list of the items, or -1 if not found.
     * @param collection the collection to parse.
     * @param itemToFind the item to find.
     */
    template<typename ITEM>
    static int findIndexOfItem(const std::vector<ITEM>& collection, const ITEM& itemToFind) noexcept
    {
        auto index = 0;
        for (const auto& item : collection) {
            if (itemToFind == item) {
                return index;
            }

            ++index;
        }

        return -1;      // Not found.
    }

    /**
     * Removes items from the given Collection IF THEY FIT the given predicate.
     * @tparam COLLECTION the collection type.
     * @tparam PREDICATE the predicate type.
     * @param collection the collection.
     * @param removeIf the predicate that, if true, will remove the items.
     */
    template<typename COLLECTION, typename PREDICATE>
    static void removeIfAndResize(COLLECTION& collection, const PREDICATE& removeIf) noexcept
    {
        // Removes the ones that fit the predicate.
        auto iterator = std::remove_if(collection.begin(), collection.end(), removeIf);
        // Removes what remains. C++...
        collection.erase(iterator, collection.end());
    }

    /**
     * Shrinks the given collection, if it is bigger than the given size.
     * @tparam COLLECTION the collection type.
     * @param collection the collection to shrink.
     * @param maximumSize the maximum the collection must have.
     */
    template<typename COLLECTION>
    static void shrinkIfBigger(COLLECTION& collection, size_t maximumSize) noexcept
    {
        if (collection.size() > maximumSize) {
            collection.resize(maximumSize);
        }
    }

    /**
     * Finds an item in the given collection, returning its index.
     * @tparam COLLECTION the collection type.
     * @tparam ITEM the item type.
     * @param collection the collection to browse.
     * @param itemToFind the item to find.
     * @return the index, or -1 if the item could not be found.
     */
    template<typename COLLECTION, typename ITEM>
    static int findItemAndGetIndex(const COLLECTION& collection, const ITEM& itemToFind) noexcept
    {
        auto index = 0;
        // Browses the collection. Have to do it "manually" in order to get the index.
        for (auto iterator = collection.cbegin(); iterator != collection.cend(); ++iterator, ++index) {
            if (*iterator == itemToFind) {
                // Founded the item! We can leave.
                return index;
            }
        }

        // Not found!
        return -1;
    }

    /**
     * @return the next or previous item of a Collection, from a desired one, and loops in the collection. If not found, asserts and returns the same one.
     * @param collection the collection to browse.
     * @param itemToFind the item to find. It should exist.
     * @param gotoNext true to get the next, false for the previous.
     */
    template<typename COLLECTION, typename ITEM>
    static ITEM getNextOrPrevious(const COLLECTION& collection, const ITEM& itemToFind, const bool gotoNext) noexcept
    {
        // Finds the current item.
        auto iterator = std::find(collection.begin(), collection.end(), itemToFind);
        if (iterator == collection.cend()) {
            jassertfalse;
            return itemToFind;
        }

        // Not elegant, but...
        if (gotoNext) {
            ++iterator;
            if (iterator == collection.cend()) {
                iterator = collection.cbegin();
            }
        } else {
            if (iterator == collection.cbegin()) {
                iterator = collection.cend() - 1;
            } else {
                --iterator;
            }
        }

        return *iterator;
    }
};

}   // namespace arkostracker
