#include "MainDocumentWindow.h"

#include "../../utils/NumberUtil.h"

namespace arkostracker 
{

MainDocumentWindow::MainDocumentWindow(const juce::String& name) noexcept :
    CustomDocumentWindow(name, true),
    mainComponent()
{
}

void MainDocumentWindow::closeButtonPressed()
{
    jassert(mainComponent != nullptr);
    if (mainComponent != nullptr) {
        mainComponent->tryToExitApplication();
    }
}

void MainDocumentWindow::showMainWindow(std::unique_ptr<juce::ApplicationCommandManager> applicationCommandManager, std::unique_ptr<Song> loadedSong) noexcept
{
    mainComponent = new MainComponent(std::move(applicationCommandManager), std::move(loadedSong));

    // Gets the possibly stored size/fullscreen flag.
    const auto windowSize = getWindowSizeFromPreferences();
    mainComponent->setSize(windowSize.getWidth(), windowSize.getHeight());

    setContentOwned(mainComponent, true);        // Ownership to JUCE.

    setUp(*mainComponent, minimumWindowWidth, minimumWindowHeight);

    // Makes sure to do that AFTER the size is set, to be sure the "non fullscreen" size if well set.
    setFullScreen(windowSize.isFullscreen());
}

MainDocumentWindow::~MainDocumentWindow()
{
    // Saves some data only this object has: the fullscreen flag.

    // A trick is used: removes the fullscreen (keeping the current fullscreen flag FIRST)
    // to be able to retrieve the window size when it isn't in fullscreen (only way to retrieve it).
    const auto fullScreen = isFullScreen();
    setFullScreen(false);

    auto& preferencesManager = PreferencesManager::getInstance();
    const auto width = mainComponent->getWidth();      // Don't use the current Document size: it includes the border and title bar.
    const auto height = mainComponent->getHeight();
    preferencesManager.storeMainWindowSize(width, height, fullScreen);
}

PreferencesManager::MainWindowSize MainDocumentWindow::getWindowSizeFromPreferences() noexcept
{
    auto& preferencesManager = PreferencesManager::getInstance();
    const auto windowSize = preferencesManager.getMainWindowSize();
    auto width = windowSize.getWidth();
    auto height = windowSize.getHeight();

    // A dimension may be 0 (unknown). If that's the case, use a portion of the screen.
    const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    const auto screenWidth = (display == nullptr) ? minimumWindowWidth : display->userArea.getWidth();
    const auto screenHeight = (display == nullptr) ? minimumWindowHeight : display->userArea.getHeight();
    constexpr auto ratio = 2.0 / 3.0;
    if (width <= 0) {
        width = static_cast<int>(static_cast<float>(screenWidth) * ratio);
    }
    if (height <= 0) {
        height = static_cast<int>(static_cast<float>(screenHeight) * ratio);
    }

    // Corrects if necessary.
    width = NumberUtil::correctNumber(width, minimumWindowWidth, screenWidth);
    height = NumberUtil::correctNumber(height, minimumWindowHeight, screenHeight);

    return { width, height, windowSize.isFullscreen() };
}

}   // namespace arkostracker
