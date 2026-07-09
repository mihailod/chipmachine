#include "MetersPanel.h"

#include "controller/MetersMainController.h"
#include "controller/MultiMeterControllerImpl.h"
#include "BinaryData.h"
#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

MetersPanel::MetersPanel(MainController& pMainController, Listener& pListener) noexcept :
        Panel(pListener),
        metersController(std::make_unique<MetersMainController>(*this, pMainController, *this)),
        customMouseListener(*this),
        switchButton(BinaryData::IconMeters_png, static_cast<size_t>(BinaryData::IconMeters_pngSize), juce::Label::ColourIds::textColourId)
{
    addChildComponent(switchButton);

    switchButton.onClick = [&] (bool) {
        metersController->onUserWantsToSwitchView();
    };
    switchButton.setInterceptsMouseClicks(true, false);
    switchButton.setMouseClickGrabsKeyboardFocus(false);

    addMouseListener(&customMouseListener, true);
}

PanelType MetersPanel::getType() const noexcept
{
    return PanelType::meters;
}

void MetersPanel::resized()
{
    metersController->updateViewLocations(getXInsideComponent(), getYInsideComponent(), getAvailableWidthInComponent(), getAvailableHeightInComponent());

    const auto margins = LookAndFeelConstants::margins;
    constexpr auto iconSize = 16;
    switchButton.setBounds(margins, /*getHeight() -*/ margins /*- iconSize*/, iconSize, iconSize);
}

void MetersPanel::getKeyboardFocus() noexcept
{
    // Nothing to do, we don't want focus.
}


// Component method implementations.
// ================================

void MetersPanel::CustomMouseListener::mouseEnter(const juce::MouseEvent& /*event*/)
{
    parentObject.switchButton.setVisible(true);
}

void MetersPanel::CustomMouseListener::mouseExit(const juce::MouseEvent& /*event*/)
{
    parentObject.switchButton.setVisible(false);
}


// MetersListener method implementations.
// =======================================

void MetersPanel::onMeterSwitchPerformed() noexcept
{
    switchButton.toFront(false);    // Else it disappears!
}

}   // namespace arkostracker
