#include "DraggedItem.h"

namespace arkostracker 
{

DraggedItem::DraggedItem(int pSourceRowIndex, ItemType pItemType) noexcept :
        sourceRowIndex(pSourceRowIndex),
        itemType(pItemType)
{
}


}   // namespace arkostracker

