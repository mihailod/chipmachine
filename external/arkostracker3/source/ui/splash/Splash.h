#pragma once

#include <memory>

#include "SplashDialog.h"

namespace arkostracker 
{

class LookAndFeelChanger;
class MainController;

/** Shows the splash screen (one for the keyboard layout, or the one for the tips), or not! */
class Splash
{
public:
    /**
     * Constructor.
     * @param lookAndFeelChanger object that knows how to change the theme.
     * @param mainController the Main Controller.
     */
    Splash(LookAndFeelChanger& lookAndFeelChanger, MainController& mainController);

    /**
     * Shows the splash, if the user decided in the preferences.
     * @return true if the splash is shown.
     */
    bool showSplashIfPossible() noexcept;

    /** Can be set to be notified of the closing of the Splash. */
    std::function<void()> onSplashClosed;

private:
    /** Called when the user closes the splash screen. */
    void onSplashFinished() noexcept;

    LookAndFeelChanger& lookAndFeelChanger;
    MainController& mainController;
    std::unique_ptr<SplashDialog> splash;
};

}   // namespace arkostracker
