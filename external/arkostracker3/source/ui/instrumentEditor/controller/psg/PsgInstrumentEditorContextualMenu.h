#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

class PsgInstrumentEditorContextualMenu
{
public:
    /**
     * Constructor.
     * @param commandManager the command manager.
     */
    explicit PsgInstrumentEditorContextualMenu(juce::ApplicationCommandManager& commandManager) noexcept;

    /**
     * Shows the contextual menu.
     * @param parent the parent, else the pop-up will provoke an unfocus-refocus.
     * @param onMenuItemSelected called when an item is selected.
     * */
    void openContextualMenu(juce::Component* parent, const std::function<void()>& onMenuItemSelected) noexcept;

private:
    juce::ApplicationCommandManager& commandManager;
    std::unique_ptr<juce::PopupMenu> popupMenu;
    std::unique_ptr<juce::PopupMenu> generateSubMenu;
};

}   // namespace arkostracker
