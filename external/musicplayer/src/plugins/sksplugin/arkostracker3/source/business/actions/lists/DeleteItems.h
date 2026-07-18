#pragma once

#include <memory>



#include "../../../utils/Remapper.h"

namespace arkostracker 
{

/** Generic abstract action to move items. This also shifts the others and modifies the song. */
template<typename ITEM>
class DeleteItems
{
public:
    /**
     * Constructor.
     * @param pIndexesToDelete the indexes to delete.
     */
    explicit DeleteItems(std::set<int> pIndexesToDelete) noexcept:
            indexesToDelete(std::move(pIndexesToDelete)),
            remapper()
    {
    }

    bool performAction() noexcept
    {
        // Anything to do?
        if (indexesToDelete.empty()) {
            return false;
        }

        const auto itemCount = getItemCount();

        // Creates the remapper, stores it to use it for Undo.
        remapper = std::move(Remapper<ITEM>::buildForDelete(itemCount, indexesToDelete));
        const auto success = performActionOnMapper(*remapper);

        // Selects the first selected item. It will be corrected if needed.
        setSelectedItem(*indexesToDelete.cbegin(), true);

        return success;
    }

    bool undoAction() noexcept
    {
        // Performs the Undo on the items.
        const auto success = undoActionOnMapper(*remapper);

        // Selects the first selected item. It will be corrected if needed.
        setSelectedItem(*indexesToDelete.cbegin(), true);

        return success;
    }

protected:
    /** @return how many items there are. */
    virtual int getItemCount() const noexcept = 0;

    /**
     * Selects an item. It will be corrected.
     * @param index the index (>0).
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after items are deleted, for example.
     */
    virtual void setSelectedItem(int index, bool forceSingleSelection) noexcept = 0;

    /** Performs the action using the given remapper, which must have been setup before. */
    virtual bool performActionOnMapper(Remapper<ITEM>& remapper) noexcept = 0;
    /** Undoes the action using the given remapper, which must have been setup before. */
    virtual bool undoActionOnMapper(Remapper<ITEM>& remapper) noexcept = 0;

private:
    const std::set<int> indexesToDelete;
    std::unique_ptr<Remapper<ITEM>> remapper;         // Will do the heavy-lifting of moving the items.
};

}   // namespace arkostracker

