#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** A (rather) generic object for the drag'n'drop in a list. */
class DraggedItem final : public juce::ReferenceCountedObject
{
public:
    /** The type of the item that is being dropped. */
    enum class ItemType
    {
        arpeggio,
        pitch,
        instrument
    };

    /**
     * Constructor.
     * @param sourceRowIndex the index of the row being dragged (>=0).
     * @param itemType the type of the item being dragged.
     */
    explicit DraggedItem(int sourceRowIndex, ItemType itemType) noexcept;

    const int sourceRowIndex;                       // The index of the row being dragged (>=0).
    const ItemType itemType;                        // The type of the item being dragged.
};

}   // namespace arkostracker

