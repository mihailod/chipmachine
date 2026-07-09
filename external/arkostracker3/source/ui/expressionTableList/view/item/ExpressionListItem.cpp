#include "ExpressionListItem.h"

#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ExpressionListItem::ExpressionListItem(int pRowNumber, bool pIsRowSelected, DraggedItem::ItemType pDraggedItemType,
                                       ListItemListener& pListener, DataProvider& pDataProvider) noexcept :
        ListItem(pRowNumber, pIsRowSelected, pDraggedItemType, pListener, pDataProvider)
{
}

bool ExpressionListItem::isDragAllowed(int pRowNumber) const noexcept
{
    return (pRowNumber > 0);
}

bool ExpressionListItem::isDropAllowedHere(int pRowNumber) const noexcept
{
    return (pRowNumber > 0);
}

juce::Colour ExpressionListItem::getBackgroundColorForEvenItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    LookAndFeelConstants::Colors color;    // NOLINT(*-init-variables)
    switch (getDraggedItemType()) {
        case DraggedItem::ItemType::instrument:
            [[fallthrough]];
        default:
            jassertfalse;           // Shouldn't happen.
        case DraggedItem::ItemType::arpeggio:
            color = LookAndFeelConstants::Colors::arpeggioListItemBackgroundEven;
            break;
        case DraggedItem::ItemType::pitch:
            color = LookAndFeelConstants::Colors::pitchListItemBackgroundEven;
            break;
    }

    return pLookAndFeel.findColour(static_cast<int>(color));
}

juce::Colour ExpressionListItem::getBackgroundColorForOddItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    LookAndFeelConstants::Colors color;    // NOLINT(*-init-variables)
    switch (getDraggedItemType()) {
        case DraggedItem::ItemType::instrument:
            [[fallthrough]];
        default:
            jassertfalse;           // Shouldn't happen.
        case DraggedItem::ItemType::arpeggio:
            color = LookAndFeelConstants::Colors::arpeggioListItemBackgroundOdd;
            break;
        case DraggedItem::ItemType::pitch:
            color = LookAndFeelConstants::Colors::pitchListItemBackgroundOdd;
            break;
    }

    return pLookAndFeel.findColour(static_cast<int>(color));
}

juce::Colour ExpressionListItem::getBackgroundColorForSelectedItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    LookAndFeelConstants::Colors color;    // NOLINT(*-init-variables)
    switch (getDraggedItemType()) {
        case DraggedItem::ItemType::instrument:
            [[fallthrough]];
        default:
            jassertfalse;           // Shouldn't happen.
        case DraggedItem::ItemType::arpeggio:
            color = LookAndFeelConstants::Colors::arpeggioListItemBackgroundSelected;
            break;
        case DraggedItem::ItemType::pitch:
            color = LookAndFeelConstants::Colors::pitchListItemBackgroundSelected;
            break;
    }

    return pLookAndFeel.findColour(static_cast<int>(color));
}


}   // namespace arkostracker

