#include "ExpressionEditorContextualMenu.h"

#include "../../keyboard/CommandIds.h"

namespace arkostracker 
{

ExpressionEditorContextualMenu::ExpressionEditorContextualMenu(juce::ApplicationCommandManager& pCommandManager) noexcept :
        commandManager(pCommandManager),
        popupMenu()
{
}

void ExpressionEditorContextualMenu::openContextualMenu(juce::Component* parent, const std::function<void()>& onMenuItemSelected) noexcept
{
    // The main menu.
    popupMenu = std::make_unique<juce::PopupMenu>();
    popupMenu->addCommandItem(&commandManager, CommandIds::barEditorInsert, juce::translate("Insert"));
    popupMenu->addCommandItem(&commandManager, CommandIds::barEditorDelete, juce::translate("Delete"));

    const juce::PopupMenu::Options options;
    // Uses a parent, else flickering. It must be one with an ApplicationCommandTarget, else the command call fails.
    popupMenu->showMenuAsync(options.withParentComponent(parent), [onMenuItemSelected](int) {
        onMenuItemSelected();   // Notifies the caller to manage the hovering.
    });
}

}   // namespace arkostracker
