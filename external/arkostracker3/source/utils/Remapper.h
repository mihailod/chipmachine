#pragma once

#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>


#include "OptionalValue.h"
#include "SetUtil.h"

namespace arkostracker 
{

/**
 * Class that builds a map useful for changing the order of items (move, delete, insert, etc.).
 * It can then reorder the items of a given Vector.
 */
template<typename TYPE>
class Remapper
{
public:

    /** What operation is performed. */
    enum class Operation : juce::uint8
    {
        MOVE,
        DELETE,
        INSERT
    };

    /** Constructor. Do not use it directly, use the builders. */
    Remapper(const Operation pOperation, const int targetIndex, std::set<int> pOriginalChangedIndexes, std::unordered_map<int, OptionalInt> pGeneratedMapping,
             std::vector<std::unique_ptr<TYPE>> pNewItemsToInsert = { }) noexcept :
            operation(pOperation),
            originalTargetIndex(targetIndex),
            originalChangedIndexes(std::move(pOriginalChangedIndexes)),
            mapping(std::move(pGeneratedMapping)),
            deletedItems(),
            itemsToInsert(std::move(pNewItemsToInsert)),
            itemsToInsertCount(static_cast<int>(itemsToInsert.size()))
    {
    }

    /**
     * Builds a Mapper when wanting to delete items.
     * @param itemCount how many items there are.
     * @param indexesToDelete the indexes of the items to delete.
     * @return an instance for the mapper.
     */
    static std::unique_ptr<Remapper> buildForDelete(const int itemCount, const std::set<int>& indexesToDelete) noexcept
    {
        // Builds the map. Deleted entries are added as Absent.
        decltype(mapping) map;
        auto outputIndex = 0;
        for (auto inputIndex = 0; inputIndex < itemCount; ++inputIndex) {
            // If this item doesn't belong to the ones to delete, stores it.
            if (indexesToDelete.find(inputIndex) == indexesToDelete.cend()) {
                // Item to keep.
                map.insert(std::make_pair(inputIndex, outputIndex));
                ++outputIndex;
            } else {
                // Item to delete. Don't increase the output index, as no instrument slot is filled.
                map.insert(std::make_pair(inputIndex, OptionalInt()));
            }
        }

        return std::make_unique<Remapper>(Operation::DELETE, -1, indexesToDelete, map);
    }

    /**
     * Builds a Mapper when wanting to move items. When moved, the items are all put consecutively.
     * @param itemCount how many items there are.
     * @param itemIndexes the indexes of the items to move.
     * @param originalTargetIndex the target index.
     * @return an instance for the mapper.
     */
    static std::unique_ptr<Remapper> buildForMove(const int itemCount, const std::set<int>& itemIndexes, const int originalTargetIndex) noexcept
    {
        const auto itemToMoveCount = static_cast<int>(itemIndexes.size());

        // Shifts the target index, according to the moved indexed before it.
        auto targetIndex = shiftIndexIfOtherIndexesMovedBefore(originalTargetIndex, itemIndexes);

        // Builds the map.
        decltype(mapping) map;
        for (auto inputIndex = 0; inputIndex < itemCount; ++inputIndex) {
            // If this item belongs to the ones to move, do it.
            if (itemIndexes.find(inputIndex) != itemIndexes.cend()) {
                // Item to move. Put it to the target, and increases it for the next item to move.
                map.insert(std::make_pair(inputIndex, targetIndex));
                ++targetIndex;
            } else {
                // This is a "normal" item. Maybe it will be shifted "towards 0", or after the moved values.
                auto newInputIndex = shiftIndexIfOtherIndexesMovedBefore(inputIndex, itemIndexes);
                // However, if it is after the insertion target, the index is going to be shifted after.
                if (inputIndex >= originalTargetIndex) {        // Checks with the original index!
                    newInputIndex += itemToMoveCount;
                }
                map.insert(std::make_pair(inputIndex, newInputIndex));
            }
        }

        return std::make_unique<Remapper>(Operation::MOVE, originalTargetIndex, itemIndexes, map);
    }

    /**
     * Builds a Mapper when wanting to add one item.
     * @param itemCount how many items there are.
     * @param indexWhereToInsert the index where to insert the new item.
     * @param newItemsToInsert the items to insert.
     * @return an instance for the mapper.
     */
    static std::unique_ptr<Remapper> buildForInsert(const int itemCount, const int indexWhereToInsert, std::vector<std::unique_ptr<TYPE>> newItemsToInsert) noexcept
    {
        // Builds the map.
        decltype(mapping) map;
        const auto itemsToInsertCount = static_cast<int>(newItemsToInsert.size());

        for (auto inputIndex = 0; inputIndex < itemCount; ++inputIndex) {
            // Indexed that are equal or above the inserted are incremented.
            const auto newIndex = (inputIndex < indexWhereToInsert) ? inputIndex : inputIndex + itemsToInsertCount;
            map.insert(std::make_pair(inputIndex, newIndex));
        }

        std::set<int> emptyMap;     // Not used.
        return std::make_unique<Remapper>(Operation::INSERT, indexWhereToInsert, emptyMap, map, std::move(newItemsToInsert));
    }

    /**
     * @return the mapping, from original indexes to remapped indexes. Deleted items are marked as absent.
     * Absent items may be let as-is or deleted, the caller decides.
     */
    const std::unordered_map<int, OptionalInt>& getMapping() const noexcept
    {
        return mapping;
    }

    /**
     * Applies the operation set in the constructor. on the given vector.
     * @param vector the vector. Will be modified.
     */
    void applyOnVector(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        switch (operation) {
            default:
                jassertfalse;
                break;
            case Operation::DELETE:
                applyOnVectorForDelete(vector);
                break;
            case Operation::MOVE:
                applyOnVectorForMove(vector);
                break;
            case Operation::INSERT:
                applyOnVectorForInsert(vector);
                break;
        }
    }

    /**
     * Undoes the operation set in the constructor. on the given vector, on which the operation has been applied before.
     * @param vector the vector. Will be modified.
     */
    void undoOnVector(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        switch (operation) {
            default:
                jassertfalse;
                break;
            case Operation::DELETE:
                undoOnVectorForDelete(vector);
                break;
            case Operation::MOVE:
                undoOnVectorForMove(vector);
                break;
            case Operation::INSERT:
                undoOnVectorForInsert(vector);
                break;
        }
    }

    /**
     * @return a modified index, if the given indexes to move are BEFORE it.
     * @param originalIndex the input index.
     * @param movedIndexes the indexes that must be moved.
     */
    static int shiftIndexIfOtherIndexesMovedBefore(const int originalIndex, const std::set<int>& movedIndexes) noexcept
    {
        jassert(originalIndex >= 0);

        auto index = originalIndex;
        for (const auto movedIndex : movedIndexes) {
            jassert(movedIndex >= 0);

            if (movedIndex < originalIndex) {
                --index;
            } else {
                // Optimization: as the set is ordered, as soon as the index is equal or superior, we can exit.
                break;
            }
        }

        return index;
    }


    // ========================================================================

private:

    /**
     * Applies the delete. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector to change.
     */
    void applyOnVectorForDelete(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::DELETE);

        jassert(deletedItems.empty());          // Method called twice??
        deletedItems.clear();

        removeFromVector(originalChangedIndexes, vector, deletedItems);
    }

    /**
     * Undoes the delete. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector. It must be the same as the one which the Do operation has been performed!
     */
    void undoOnVectorForDelete(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::DELETE);

        jassert(originalChangedIndexes.size() == deletedItems.size());      // Big trouble if not the case!

        // Puts back the items, in order.
        auto iterator = deletedItems.begin();
        for (const auto indexOfDeletedItem : originalChangedIndexes) {
            // Transfers ownership. The original list is cleared below, so this is safe.
            vector.insert(vector.cbegin() + indexOfDeletedItem, std::move(*iterator));

            ++iterator;
        }

        deletedItems.clear();
    }

    /**
     * Applies the move. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector to change.
     */
    void applyOnVectorForMove(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::MOVE);

        // This code is complicated by the fact that all the items are unique_ptr, so we have to release them to get them...

        // Removes the selected items, stores them temporarily.
        std::vector<std::unique_ptr<TYPE>> tempStorage;
        removeFromVector(originalChangedIndexes, vector, tempStorage);

        // Adds them to the original collection, at a different place.
        auto targetIndex = shiftIndexIfOtherIndexesMovedBefore(originalTargetIndex, originalChangedIndexes);
        for (auto& item : tempStorage) {
            auto extractedItem = std::unique_ptr<TYPE>(item.release());      // This screws the temp collection, we don't care, it is a temp.

            vector.insert(vector.begin() + targetIndex, std::move(extractedItem));
            ++targetIndex;
        }
    }

    /**
     * Undoes the move. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector. It must be the same as the one which the Do operation has been performed!
     */
    void undoOnVectorForMove(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::MOVE);

        // Removes the items that has been moved contiguously, stores them temporarily.
        // First, creates the target indexes. Warning, the index may need to be shifted.
        const auto targetIndex = shiftIndexIfOtherIndexesMovedBefore(originalTargetIndex, originalChangedIndexes);
        const auto targetIndexes = SetUtil::createLinearItems<int>(targetIndex, false, static_cast<int>(originalChangedIndexes.size()));

        std::vector<std::unique_ptr<TYPE>> tempStorage;
        removeFromVector(targetIndexes, vector, tempStorage);

        // Adds them to the original collection, at their original location.
        auto indexesIterator = originalChangedIndexes.begin();
        for (auto& item : tempStorage) {
            auto extractedItem = std::unique_ptr<TYPE>(item.release());      // This screws the temp collection, we don't care, it is a temp.

            vector.insert(vector.begin() + *indexesIterator, std::move(extractedItem));
            ++indexesIterator;
        }
    }


    /**
     * Applies the insert. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector to change.
     */
    void applyOnVectorForInsert(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::INSERT);

        // Extracts the unique_ptr from the original list, before adding them.
        auto index = originalTargetIndex;
        for (auto& itemToInsert : itemsToInsert) {
            auto itemUnique = std::unique_ptr<TYPE>(itemToInsert.release());        // The original collection becomes dandling, we don't need it anymore.

            vector.insert(vector.cbegin() + index, std::move(itemUnique));
            ++index;
        }

        itemsToInsert.clear();              // We don't need them anymore, plus the collection has moved items, so dangerous. The counter is kept, though.
    }

    /**
     * Undoes the Insert. WARNING. as the collection is modified, it is up to the client to manage all
     * concurrent aspect (locking, etc.).
     * @param vector the vector. It must be the same as the one which the Do operation has been performed!
     */
    void undoOnVectorForInsert(std::vector<std::unique_ptr<TYPE>>& vector) noexcept
    {
        jassert(operation == Operation::INSERT);

        vector.erase(vector.cbegin() + originalTargetIndex, vector.cbegin() + originalTargetIndex + itemsToInsertCount);

        itemsToInsertCount = 0;
    }


    /**
     * Removes the items from a given collection, and fills another collection with the removed items.
     * @param indexOfItemsToRemove the indexes of the items to remove.
     * @param collectionToRemoveFrom the collection where the items must be removed.
     * @param collectionToFill will receive the removed items.
     */
    static void removeFromVector(const std::set<int>& indexOfItemsToRemove, std::vector<std::unique_ptr<TYPE>>& collectionToRemoveFrom,
                                 std::vector<std::unique_ptr<TYPE>>& collectionToFill) noexcept
    {
        // Removes the selected items, stores them in the class, for the undo. From END to BEGINNING!
        for (auto iterator = indexOfItemsToRemove.rbegin(); iterator != indexOfItemsToRemove.rend(); ++iterator) {
            const auto changedIndex = *iterator;
            jassert(changedIndex < (int)collectionToRemoveFrom.size());

            // Transfers the item from the original collection and puts it in the Undo collection.
            // Puts at the BEGINNING, because reading from END to BEGINNING.
            auto it = collectionToRemoveFrom.begin() + changedIndex;
            auto item = std::unique_ptr<TYPE>(it->release());
            collectionToFill.insert(collectionToFill.cbegin(), std::move(item));      // Puts at the BEGINNING, because reading from END to BEGINNING.

            // Removes it from the original collection.
            collectionToRemoveFrom.erase(collectionToRemoveFrom.cbegin() + changedIndex);
        }
    }

    Operation operation;
    const int originalTargetIndex;                      // The original target index, if relevant.
    const std::set<int> originalChangedIndexes;         // The original changed indexes (moved, selected, etc.).
    std::unordered_map<int, OptionalInt> mapping;       // From input to remapped indexes. Deleted items are encoded as Absent. Not encoded items may be let as is.

    std::vector<std::unique_ptr<TYPE>> deletedItems;    // The deleted items, stored until undo.

    std::vector<std::unique_ptr<TYPE>> itemsToInsert;   // The stored Items to be inserted.
    int itemsToInsertCount;                             // How many items were inserted.

    JUCE_LEAK_DETECTOR (Remapper)
};


}   // namespace arkostracker

