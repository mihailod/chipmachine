#include "PsgInstrumentEditorContextualMenu.h"

#include "../../../keyboard/CommandIds.h"

namespace arkostracker 
{

PsgInstrumentEditorContextualMenu::PsgInstrumentEditorContextualMenu(juce::ApplicationCommandManager& pCommandManager) noexcept :
        commandManager(pCommandManager),
        popupMenu(),
        generateSubMenu()
{
}

void PsgInstrumentEditorContextualMenu::openContextualMenu(juce::Component* parent, const std::function<void()>& onMenuItemSelected) noexcept
{
    // First, the "generate" submenu.
    generateSubMenu = std::make_unique<juce::PopupMenu>();
    generateSubMenu->addCommandItem(&commandManager, CommandIds::psgBarEditorGenerateDecreasingVolume, juce::translate("Decreasing volume (overwrite)"));
    generateSubMenu->addCommandItem(&commandManager, CommandIds::psgBarEditorGenerateIncreasingVolume, juce::translate("Increasing volume (overwrite)"));

    // The main menu.
    popupMenu = std::make_unique<juce::PopupMenu>();
    popupMenu->addCommandItem(&commandManager, CommandIds::barEditorInsert, juce::translate("Insert"));
    popupMenu->addCommandItem(&commandManager, CommandIds::barEditorDelete, juce::translate("Delete"));
    popupMenu->addSeparator();
    popupMenu->addCommandItem(&commandManager, CommandIds::psgBarEditorToggleRetrig, juce::translate("Toggle retrig"));
    popupMenu->addSeparator();
    popupMenu->addSubMenu(juce::translate("Generate..."), *generateSubMenu);

    const juce::PopupMenu::Options options;
    // Uses a parent, else flickering. It must be one with an ApplicationCommandTarget, else the command call fails.
    popupMenu->showMenuAsync(options.withParentComponent(parent), [onMenuItemSelected](int) {
        onMenuItemSelected();   // Notifies the caller to manage the hovering.
    });
}

}   // namespace arkostracker
