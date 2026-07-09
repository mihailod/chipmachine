#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "FrequencyMeterController.h"
#include "MetersController.h"

namespace arkostracker
{

class MainController;
class MetersListener;

/** Main Meters controller. Holds the one to display. */
class MetersMainController final : public MetersController
{
public:
    MetersMainController(juce::Component& parentComponent, MainController& mainController, MetersListener& metersListener) noexcept;

    // MetersController method implementations.
    // ==============================================
    void updateViewLocations(int startX, int startY, int width, int height) override;
    void onWantToChangeMuteState(int channelIndex, bool newMuteState) override;
    void onWantToAllMuteExcept(int channelIndex) override;
    void onUserWantsToSwitchView() noexcept override;

private:
    enum class MeterType
    {
        meters,
        frequencies,
    };

    static MeterType currentMeterType;          // Quick hack as a static, because this is re-created when changing layout. No need to make something more complicated...

    /** Creates the view controller according to the current meter view state. */
    void createViewController();

    juce::Component& parentComponent;
    MainController& mainController;
    MetersListener& metersListener;
    juce::Rectangle<int> currentLocation;

    std::unique_ptr<MetersController> currentController;
};

}   // namespace arkostracker
