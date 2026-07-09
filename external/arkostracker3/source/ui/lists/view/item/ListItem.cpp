#include "ListItem.h"

#include "../../../../utils/NumberUtil.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../controller/DataProvider.h"

namespace arkostracker 
{

ListItem::ListItem(const int pRowNumber, const bool pIsRowSelected, const DraggedItem::ItemType pDraggedItemType,
                   ListItemListener& pListener, DataProvider& pDataProvider) noexcept :
        AbstractListItem(pRowNumber, pIsRowSelected, pDraggedItemType, pListener),
        dataProvider(pDataProvider)
{
}


// Component method implementations.
// =================================================

void ListItem::paint(juce::Graphics& g)
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    const auto localIsRowSelected = isRowSelected();
    const auto localRowNumber = getRowNumber();

    // What background color?
    const auto backgroundColor =
            localIsRowSelected ? getBackgroundColorForSelectedItems(localLookAndFeel) :
            (((localRowNumber % 2) == 0) ? getBackgroundColorForEvenItems(localLookAndFeel) : getBackgroundColorForOddItems(localLookAndFeel));
    g.fillAll(backgroundColor);

    // Displays the text.
    const auto& textColor = localLookAndFeel.findColour(
            static_cast<int>(localIsRowSelected ? LookAndFeelConstants::Colors::listItemTextSelected : LookAndFeelConstants::Colors::listItemText)
    );
    g.setColour(textColor);

    const auto horizontalMargin = LookAndFeelConstants::margins;
    const int textWidth = width - 2 * horizontalMargin;
    const auto fontHeight = LookAndFeelConstants::listItemFontSize;
    const auto textY = (height - fontHeight) / 2;
    g.setFont(static_cast<float>(fontHeight));
    drawText(g, localRowNumber, width, height, horizontalMargin, textY, textWidth, fontHeight);

    // More customization?
    drawOnGraphics(g, localRowNumber, width, height, textY, fontHeight);

    // Shows small vertical line is selected.
    if (localIsRowSelected) {
        const auto heightFloat = static_cast<float>(height);
        const auto lineColor = backgroundColor.interpolatedWith(textColor, 0.5F);
        constexpr auto verticalLineThickness = 2.0F;
        constexpr auto xVerticalLineLeft = verticalLineThickness / 2.0F;
        const auto xVerticalLineRight = static_cast<float>(width) - verticalLineThickness / 2.0F;
        g.setColour(lineColor);
        g.drawLine(xVerticalLineLeft, 0.0F, xVerticalLineLeft, heightFloat, verticalLineThickness);
        g.drawLine(xVerticalLineRight, 0.0F, xVerticalLineRight, heightFloat, verticalLineThickness);
    }

    // Any drag'n'drop area to highlight?
    showPossibleDragNDropHighlight(g, backgroundColor);
}

void ListItem::drawText(juce::Graphics& g, const int pRowNumber, const int /*width*/, const int /*height*/, const int suggestedTextX, const int y, const int maximumTextWidth,
                        const int textHeight) noexcept {
    const auto text = dataProvider.getNumberPrefix() + NumberUtil::toHexByte(pRowNumber) + ": " + dataProvider.getItemTitle(pRowNumber);
    g.drawText(text, suggestedTextX, y, maximumTextWidth, textHeight, juce::Justification::left, true);
}

void ListItem::drawOnGraphics(juce::Graphics& /*graphics*/, int /*number*/, const int /*width*/, const int /*height*/, const int /*textY*/, const int /*textHeight*/) noexcept
{
    // Nothing to do.
}


}   // namespace arkostracker

