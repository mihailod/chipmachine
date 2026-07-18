#pragma once

#include "../../../utils/Remapper.h"
#include "../../../utils/CollectionUtil.h"

namespace arkostracker 
{

/** Generic abstract action to add items. This also shifts the others and modifies the song. */
template<typename ITEM>
class AddItems
{
public:

    /**
     * Constructor.
     * @param targetIndex where to insert (>0!).
     * @param items the items.
     * @param originalSelectedIndex the selected index, for the undo.
     */
    AddItems(int pTargetIndex, std::vector<std::unique_ptr<ITEM>> pItems, int pOriginalSelectedIndex) noexcept :
            targetIndex(pTargetIndex),
            originalItems(std::move(pItems)),
            originalSelectedIndex(pOriginalSelectedIndex),
            remapper()
    {
    }

    bool performAction() noexcept
    {
        // Anything to add?
        if (originalItems.empty()) {
            return false;
        }

        // Performs a copy of the original items, because the original is needed for the Redo.
        auto items = CollectionUtil::copyVectorOfUniquePtr(originalItems);
        const auto itemCount = getItemCount();

        // Creates the remapper, stores it to use it for Undo.
        remapper = std::move(Remapper<ITEM>::buildForInsert(itemCount, targetIndex, std::move(items)));
        const auto success = performActionOnMapper(*remapper);

        setSelectedItem(targetIndex, true);

        return success;
    }

    bool undoAction() noexcept
    {
        const auto success = undoActionOnMapper(*remapper);

        setSelectedItem(originalSelectedIndex, true);

        return success;
    }

protected:
    /** @return how many items there are. */
    virtual int getItemCount() const noexcept = 0;

    /**
     * Selects an expression. It will be corrected.
     * @param index the index (>0).
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after expressions are deleted, for example.
     */
    virtual void setSelectedItem(int index, bool forceSingleSelection) noexcept = 0;

    /** Performs the action using the given remapper, which must have been setup before. */
    virtual bool performActionOnMapper(Remapper<ITEM>& remapper) noexcept = 0;
    /** Undoes the action using the given remapper, which must have been setup before. */
    virtual bool undoActionOnMapper(Remapper<ITEM>& remapper) noexcept = 0;

private:
    const int targetIndex;
    std::vector<std::unique_ptr<ITEM>> originalItems;
    const int originalSelectedIndex;
    std::unique_ptr<Remapper<ITEM>> remapper;         // Will do the heavy-lifting of moving the items.
};

}   // namespace arkostracker

