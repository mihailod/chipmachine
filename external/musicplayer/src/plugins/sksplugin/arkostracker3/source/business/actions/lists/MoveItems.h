#pragma once

#include <memory>
#include <set>



#include "../../../utils/Remapper.h"

namespace arkostracker 
{

/** Generic abstract action to move items. This also shifts the others and modifies the song. */
template<typename ITEM>
class MoveItems
{
public:
    /**
     * Constructor.
     * @param indexesToMove the indexes to move.
     * @param targetIndex the new index. The items will be put one after the other, in order. Does NOT take in account the fact that the expression has moved! So, matches the UI.
     */
    MoveItems(std::set<int> indexesToMove, int targetIndex) noexcept :
            originalIndexesToMove(std::move(indexesToMove)),
            originalTargetIndex(targetIndex),
            remapper()
    {
    }

    virtual ~MoveItems() = default;

    bool performAction() noexcept
    {
        // Anything to do?
        if (originalIndexesToMove.empty()) {
            return false;
        }

        const auto itemCount = getItemCount();

        // Creates the remapper, stores it to use it for Undo.
        remapper = std::move(Remapper<ITEM>::buildForMove(itemCount, originalIndexesToMove, originalTargetIndex));
        const auto success = performActionOnMapper(*remapper);

        // Selects the target index. It must be shifted in case items were moved before it.
        const auto selectedIndex = Remapper<ITEM>::shiftIndexIfOtherIndexesMovedBefore(originalTargetIndex, originalIndexesToMove);
        setSelectedItem(selectedIndex, true);

        return success;
    }

    bool undoAction() noexcept
    {
        // Performs the Undo on the items.
        const auto success = undoActionOnMapper(*remapper);

        // Selects the first selected item. It will be corrected if needed.
        setSelectedItem(*originalIndexesToMove.cbegin(), true);

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
    const std::set<int> originalIndexesToMove;
    const int originalTargetIndex;
    std::unique_ptr<Remapper<ITEM>> remapper;         // Will do the heavy-lifting of moving the items.
};

}   // namespace arkostracker
