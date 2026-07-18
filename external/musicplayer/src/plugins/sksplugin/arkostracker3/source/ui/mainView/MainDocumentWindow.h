#pragma once

#include <memory>

#include "../../app/preferences/PreferencesManager.h"
#include "../components/CustomDocumentWindow.h"
#include "MainComponent.h"

namespace arkostracker 
{

class MainComponent;

/** This holds the Main Component. */
class MainDocumentWindow final : public CustomDocumentWindow
{
public:
    /**
     * Constructor. This does NOT show the main window. Use the ShowMainWindow for that.
     * @param name the name to show at the top.
     */
    explicit MainDocumentWindow(const juce::String& name) noexcept;

    /** Destructor. */
    ~MainDocumentWindow() override;

    /**
     * Displays the Main Window.
     * @param applicationCommandManager the ApplicationCommandManager to use. It should be set-up with ALL the commands of all the panels and main Component.
     * @param loadedSong the possible loaded song, or nullptr.
     */
    void showMainWindow(std::unique_ptr<juce::ApplicationCommandManager> applicationCommandManager, std::unique_ptr<Song> loadedSong) noexcept;

    // DocumentWindow method implementations.
    // ================================================
    void closeButtonPressed() override;

    /** Note: Be careful if you override any DocumentWindow methods - the base
    class uses a lot of them, so by overriding you might break its functionality.
    It's best to do all your work in your content component instead, but if
    you really have to override any DocumentWindow methods, make sure your
    subclass also calls the superclass's method.
    */

private:
    static constexpr auto minimumWindowWidth = 600;                      // Below that value, consider the size too small.
    static constexpr auto minimumWindowHeight = 400;                     // Below that value, consider the size too small.

    /** @return a valid Window Size to use, from the one possibly stored in the Preferences. */
    static PreferencesManager::MainWindowSize getWindowSizeFromPreferences() noexcept;

    MainComponent* mainComponent;                           // Where all the content is displayed. Reference only needed to manage the Close button.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainDocumentWindow) // NOLINT
};

}   // namespace arkostracker
