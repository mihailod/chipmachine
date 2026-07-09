#include "InstrumentListItem.h"

#include <BinaryData.h>

#include "../../../../utils/NumberUtil.h"
#include "../../../lists/controller/DataProvider.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"
#include "../../controller/InstrumentDataController.h"

namespace arkostracker 
{

const int InstrumentListItem::instrumentTypeViewX = 5;
const int InstrumentListItem::instrumentTypeViewWidth = 30;
const int InstrumentListItem::colorViewSupposedWidth = 40;
const int InstrumentListItem::textX = 40;
const int InstrumentListItem::textMinimumWidth = 30;

InstrumentListItem::InstrumentListItem(const int pRowNumber, const bool pIsRowSelected, ListItemListener& pListener, DataProvider& pDataProvider,
                                       InstrumentDataController& pInstrumentDataController) noexcept :
        ListItem(pRowNumber, pIsRowSelected, DraggedItem::ItemType::instrument, pListener, pDataProvider),
        instrumentDataController(pInstrumentDataController),
        instrumentTypeView(getImageColor()),
        // Use a boilerplate color, don't use controller here because the constructor is called for all viewable rows,
        // so will use out-of-range rows.
        colorView(*this, juce::Colours::transparentBlack),
        modalDialog(),
        instrumentTypeImageClickListener(*this)
{
    instrumentTypeView.addMouseListener(&instrumentTypeImageClickListener, false);

    addChildComponent(colorView);               // Set to visible or not on the drawOnGraphics method.
    addAndMakeVisible(instrumentTypeView);
}

bool InstrumentListItem::isDragAllowed(const int pRowNumber) const noexcept
{
    return (pRowNumber > 0);
}

bool InstrumentListItem::isDropAllowedHere(const int pRowNumber) const noexcept
{
    return (pRowNumber > 0);
}

juce::Colour InstrumentListItem::getBackgroundColorForEvenItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    return pLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundEven));
}

juce::Colour InstrumentListItem::getBackgroundColorForOddItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    return pLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundOdd));
}

juce::Colour InstrumentListItem::getBackgroundColorForSelectedItems(const juce::LookAndFeel& pLookAndFeel) noexcept
{
    return pLookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundSelected));
}

juce::Colour InstrumentListItem::getImageColor() noexcept
{
    return juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);
}

void InstrumentListItem::drawText(juce::Graphics& g, const int pRowNumber, const int width, int /*height*/, int /*suggestedTextX*/, const int textY,
                                  int /*maximumTextWidth*/, const int textHeight) noexcept
{
    const auto textWidth = std::max(width - colorViewSupposedWidth - textX, textMinimumWidth);        // Prevents total collapse.
    const auto text = NumberUtil::toHexByte(pRowNumber) + juce::String(": ") + dataProvider.getItemTitle(getRowNumber());
    g.drawText(text, textX, textY, textWidth, textHeight, juce::Justification::left, true);
}

void InstrumentListItem::drawOnGraphics(juce::Graphics& /*g*/, const int pRowNumber, int /*width*/, int /*height*/, int /*textY*/, int /*textHeight*/) noexcept
{
    // Updates the Components here.
    const auto color = instrumentDataController.getInstrumentColor(pRowNumber);

    const auto isColorVisible = (pRowNumber > 0);           // 0 doesn't have a visible color.
    colorView.setVisible(isColorVisible);
    if (isColorVisible) {
        colorView.setColorAndRepaint(color);
    }

    // Displays the instrument type image.
    switch (instrumentDataController.getInstrumentType(pRowNumber)) {
        default:
            jassertfalse;       // Type not managed?
        case InstrumentType::psgInstrument:
            instrumentTypeView.setImage(juce::ImageFileFormat::loadFrom(BinaryData::InstrumentTypePsg_png, BinaryData::InstrumentTypePsg_pngSize));
            break;
        case InstrumentType::sampleInstrument:
            instrumentTypeView.setImage(juce::ImageFileFormat::loadFrom(BinaryData::InstrumentTypeSample_png, BinaryData::InstrumentTypeSample_pngSize));
            break;
    }
}


// ColorView::Listener method implementations.
// ==============================================

void InstrumentListItem::onColorClicked(const juce::Colour& color, int /*viewId*/) noexcept
{
    jassert(modalDialog == nullptr);            // Modal already present? Abnormal.
    modalDialog = std::make_unique<ColorChooser>(*this, &color);
}


// ColorChooser::Listener method implementations.
// ================================================

void InstrumentListItem::onColorSelectedInColorChooser(const juce::Colour color) noexcept
{
    modalDialog.reset();

    instrumentDataController.setInstrumentColor(getRowNumber(), color);
}

void InstrumentListItem::onColorInColorChooserCancelled() noexcept
{
    modalDialog.reset();
}


// Component method implementations.
// ====================================

void InstrumentListItem::resized()
{
    const auto height = getHeight();
    const auto width = getWidth();

    constexpr auto marginX = 5;
    constexpr auto marginY = marginX;
    constexpr auto colorViewY = marginY;
    const auto colorViewHeight = height - (2 * marginY);
    const auto colorViewWidth = colorViewHeight;
    constexpr auto instrumentTypeViewY = marginY;
    const auto instrumentTypeViewHeight = colorViewHeight;
    // Prevents the ColorView on the left to go over the InstrumentType, doesn't look good. Only an approximation, but is enough.
    const auto colorViewX = std::max(width - marginX - colorViewWidth, instrumentTypeViewX + instrumentTypeViewWidth + textMinimumWidth + (2 * marginX));

    colorView.setBounds(colorViewX, colorViewY, colorViewWidth, colorViewHeight);

    instrumentTypeView.setBounds(instrumentTypeViewX, instrumentTypeViewY, instrumentTypeViewWidth, instrumentTypeViewHeight);
}

void InstrumentListItem::lookAndFeelChanged()
{
    ListItem::lookAndFeelChanged();
    instrumentTypeView.setImageColor(getImageColor());
}


// ImageClickListener methods.
// =================================

InstrumentListItem::ImageClickListener::ImageClickListener(InstrumentListItem& parent) :
        WithParent(parent)
{
}

void InstrumentListItem::ImageClickListener::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown()) {
        return;
    }

    parentObject.instrumentDataController.onInstrumentSymbolClicked(parentObject.getRowNumber());
}

}   // namespace arkostracker
