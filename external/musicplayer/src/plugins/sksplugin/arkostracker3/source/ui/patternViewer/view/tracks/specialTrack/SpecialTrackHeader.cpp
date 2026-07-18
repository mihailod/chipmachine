#include "SpecialTrackHeader.h"

#include <BinaryData.h>

namespace arkostracker 
{


// SpecialTrackHeader::DisplayedData
// ============================================================

SpecialTrackHeader::DisplayedData::DisplayedData(juce::String pSpecialTrackName, const LinkState pLinkState) noexcept :
        specialTrackName(std::move(pSpecialTrackName)),
        linkState(pLinkState)
{
}

const juce::String& SpecialTrackHeader::DisplayedData::getSpecialTrackName() const noexcept
{
    return specialTrackName;
}

LinkState SpecialTrackHeader::DisplayedData::getLinkState() const noexcept
{
    return linkState;
}

bool SpecialTrackHeader::DisplayedData::operator==(const SpecialTrackHeader::DisplayedData& rhs) const
{
    return linkState == rhs.linkState &&
           specialTrackName == rhs.specialTrackName;
}

bool SpecialTrackHeader::DisplayedData::operator!=(const SpecialTrackHeader::DisplayedData& rhs) const
{
    return !(rhs == *this);
}


// ============================================================

SpecialTrackHeader::SpecialTrackHeader(Listener& pListener) noexcept :
        listener(pListener),
        displayedData(juce::String(), LinkState::none),    // Boiler-plate for now.
        namePresentImage(juce::ImageFileFormat::loadFrom(BinaryData::TinyIconName_png, BinaryData::TinyIconName_pngSize)),
        linkedImage(juce::ImageFileFormat::loadFrom(BinaryData::TinyIconLinked_png, BinaryData::TinyIconLinked_pngSize)),
        linkedToImage(juce::ImageFileFormat::loadFrom(BinaryData::TinyIconLinkedTo_png, BinaryData::TinyIconLinkedTo_pngSize))
{
}

void SpecialTrackHeader::paintAfter(juce::Graphics& g, const juce::Colour /*backgroundColor*/) noexcept
{
    // Sets the image in the opposite color of the text.
    const auto imageColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);
    g.setColour(imageColor);

    const auto bounds = getLocalBounds().toFloat();

    const auto& image = getImage();
    g.drawImage(image, bounds.reduced(8.0F), juce::RectanglePlacement::centred, true);

    // Displays the possible tiny image.
    const auto state = determineIconState();
    if (state == IconState::none) {
        return;     // Nothing to display.
    }
    const auto& tinyImage = (state == IconState::linkedTo) ? linkedToImage
        : ((state == IconState::linked) ? linkedImage : namePresentImage);

    const auto imageRight = static_cast<int>(bounds.getRight() - 2.0F);
    const auto bottom = static_cast<int>(bounds.getBottom() - 1.0F);
    g.drawImageAt(tinyImage, imageRight - tinyImage.getWidth(), bottom - tinyImage.getHeight(), true);
}

void SpecialTrackHeader::setDisplayedData(DisplayedData newDisplayedData) noexcept
{
    if (displayedData == newDisplayedData) {
        return;
    }
    displayedData = std::move(newDisplayedData);

    setTooltip(displayedData.getSpecialTrackName());

    repaint();
}

SpecialTrackHeader::IconState SpecialTrackHeader::determineIconState() const noexcept
{
    if (displayedData.getLinkState() == LinkState::linkedTo) {
        return IconState::linkedTo;
    }

    if (displayedData.getLinkState() == LinkState::link) {
        return IconState::linked;
    }

    if (displayedData.getSpecialTrackName().isNotEmpty()) {
        return IconState::name;
    }

    return IconState::none;
}


// Component method implementations.
// ======================================

void SpecialTrackHeader::mouseDown(const juce::MouseEvent& /*event*/)
{
    juce::PopupMenu popupMenu;
    popupMenu.addItem(juce::translate("Edit name"), [&] {
        listener.onUserWantsToNameSpecialTrack(displayedData.getSpecialTrackName());
    });
    popupMenu.addItem(juce::translate("Edit link"), [&] {
        listener.onUserWantsToManageSpecialLink();
    });
    popupMenu.show();
}

}   // namespace arkostracker
