#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

class CommandPerformer;
class MainController;

/** The main menu bar, at the top (File, Edit, etc.). */
class AppMenuBarModel : public juce::MenuBarModel
{
public:
    /** Constructor. */
    AppMenuBarModel(CommandPerformer& commandPerformer, MainController& mainController);

    // MenuBarModel method implementations.
    // ========================================
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemId, int topLevelMenuIndex) override;

private:
    enum class MenuIds: uint8_t {
        file,
        edit,
        tools,
        help
    };

    static constexpr int recentFileFirstId = 0x100;         // First id for the recent files items.

    /** Subclass of the callback when a Recent File item is clicked. Holds their index. */
    class RecentFileClickCallback : public juce::PopupMenu::CustomCallback
    {
    public:
        /**
         * Constructor.
         * @param filePath the file to load.
         * @param mainController the Main Controller, to transmit the event.
         */
        RecentFileClickCallback(juce::String filePath, MainController& mainController) noexcept;

        bool menuItemTriggered() override;

        MainController& mainController;         // The Main Controller, to transmit the event.
        juce::String filePath;                  // The file to load.
    };

    /**
     * Fills the given submenu showing the recent files.
     * @param recentFilesSubMenu the menu to fill. It must be newly created and thus, empty.
     * @param commandManager the Command Manager to register the Clear List command.
     * @return true if at least one recent file is present.
     */
    bool fillRecentFilesSubMenu(juce::PopupMenu& recentFilesSubMenu, juce::ApplicationCommandManager& commandManager) const noexcept;

    CommandPerformer& commandPerformer;
    MainController& mainController;
};

}   // namespace arkostracker
