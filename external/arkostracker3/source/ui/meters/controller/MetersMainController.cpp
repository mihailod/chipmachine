#include "MetersMainController.h"

#include "FrequencyMeterController.h"
#include "MultiMeterControllerImpl.h"
#include "../MetersListener.h"

namespace arkostracker
{

MetersMainController::MeterType MetersMainController::currentMeterType = MeterType::meters;

MetersMainController::MetersMainController(juce::Component& pParentComponent, MainController& pMainController, MetersListener& pMetersListener) noexcept :
        parentComponent(pParentComponent),
        mainController(pMainController),
        metersListener(pMetersListener),
        currentLocation(),
        currentController()
{
    createViewController();
}

void MetersMainController::updateViewLocations(const int startX, const int startY, const int width, const int height)
{
    currentLocation = juce::Rectangle(startX, startY, width, height);       // Useful when switching from a view to another.
    currentController->updateViewLocations(startX, startY, width, height);
}

void MetersMainController::onWantToChangeMuteState(const int channelIndex, const bool newMuteState)
{
    currentController->onWantToChangeMuteState(channelIndex, newMuteState);
}

void MetersMainController::onWantToAllMuteExcept(const int channelIndex)
{
    currentController->onWantToAllMuteExcept(channelIndex);
}

void MetersMainController::onUserWantsToSwitchView() noexcept
{
    MeterType newMeterView;
    switch (currentMeterType) {
        default:
            jassertfalse;
        case MeterType::meters:
            newMeterView = MeterType::frequencies;
            break;
        case MeterType::frequencies:
            newMeterView = MeterType::meters;
            break;
    }

    currentMeterType = newMeterView;

    createViewController();

    metersListener.onMeterSwitchPerformed();
}

void MetersMainController::createViewController()
{
    switch (currentMeterType) {
        default:
            jassertfalse;
        case MeterType::meters:
            currentController = std::make_unique<MultiMeterControllerImpl>(parentComponent, mainController);
            break;
        case MeterType::frequencies:
            currentController = std::make_unique<FrequencyMeterController>(parentComponent, mainController);
            break;
    }

    // Applies the latest location.
    currentController->updateViewLocations(currentLocation.getX(), currentLocation.getY(), currentLocation.getWidth(), currentLocation.getHeight());
}

}   // namespace arkostracker
