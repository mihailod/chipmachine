#include "Splash.h"

#include "../../app/preferences/PreferencesManager.h"
#include "SplashTips.h"
#include "../../controllers/MainController.h"
#include "SplashKeyboard.h"

namespace arkostracker 
{

Splash::Splash(LookAndFeelChanger& pLookAndFeelChanger, MainController& pMainController) :
        onSplashClosed(),
        lookAndFeelChanger(pLookAndFeelChanger),
        mainController(pMainController),
        splash()
{
}

bool Splash::showSplashIfPossible() noexcept
{
    auto splashShown = true;

    const auto & preferences = PreferencesManager::getInstance();
    // Priority to the keyboard splash.
    if (!preferences.isVirtualKeyboardLayoutExplicitlySet()) {
        splash = std::make_unique<SplashKeyboard>(lookAndFeelChanger, mainController, [&]{ onSplashFinished(); });
        splash->show();
    } else if (preferences.isSplashShown()) {
        splash = std::make_unique<SplashTips>(mainController, [&]{ onSplashFinished(); });
        splash->show();
    } else {
        splashShown = false;
    }

    return splashShown;
}

void Splash::onSplashFinished() noexcept
{
    splash.reset();
    onSplashClosed();
}

}   // namespace arkostracker
