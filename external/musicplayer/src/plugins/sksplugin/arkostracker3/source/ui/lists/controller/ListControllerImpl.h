#pragma once

#include "../../utils/ListItemListener.h"
#include "../OperationValidityProvider.h"
#include "../view/ListView.h"
#include "../view/ListViewListener.h"
#include "ListController.h"

namespace arkostracker 
{

class MainController;

/**
 * Abstract implementation of a list controller. Subclasses will provide for the specific actions.
 *
 * This class still uses raw indexes, because JUCE ListBox does. However, the attempt is not to leak them.
 * However, this cannot be fully done, especially for the move/delete etc. actions, which are using the remapper.
 *
 * Important implementation note:
 * - The ListBox does not notify anything. Events of clicks on items are received here, which provoke
 *   a selection in the ListBox. But that's it. We manage notifying the Controller on our own.
 */
class ListControllerImpl : public ListController,
                           public ListItemListener,
                           public ListViewListener,
                           public OperationValidityProvider
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit ListControllerImpl(MainController& mainController) noexcept;

    /** Gets the keyboard focus to the view. */
    void getKeyboardFocus() noexcept override;

    // ParentViewLifeCycleAware method implementations.
    // =================================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;

    // ListItemListener method implementations.
    // ==========================================
    void onListItemClicked(int rowIndex, juce::ModifierKeys modifierKeys, bool isLeftMouseDown) override;
    void onListItemDoubleClicked(int rowIndex) override;
    void onListItemRightClicked(int rowIndex) override;
    void onDragListItem(int sourceRowIndex, int destinationRowIndex) override;
    void onDeleteKeyPressed() override;
    void onReturnKeyPressed() override;
    void onListItemSelectionChanged() override;

    // ListViewListener method implementations.
    // =========================================================
    void onUserConfirmedRename(int itemIndex, const juce::String& newName) noexcept override;
    void onUserWantsToAdd() noexcept override;
    void onUserWantsToDelete(const std::set<int>& originalIndexesToDelete) noexcept override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) noexcept override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) noexcept override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept override;

    // OperationValidityProvider method implementations.
    // ================================================
    bool canOperationBePerformed(int commandId) const noexcept override;

protected:
    /** Updates the full list. */
    void updateList() const noexcept;

    /**
     * Updates an item which has been updated.
     * @param itemIndex the index (>=1, 0 is never shown).
     */
    void updateItem(int itemIndex) const noexcept;

    /** Updates the list selection from the current data. This does not trigger a notification. */
    void updateShownSelectedRows() const noexcept;

    /** Call this when the selection changes (detected from the mouse, or keyboard). Stores the new selection, notifies the Controller. */
    void manageSelectionChanged() noexcept;

    /** Shows a pop-up menu (add, delete, etc.). */
    void showPopUpMenu() noexcept;

    /**
     * Selects one row, and notifies the Controller. This class will receive a notification.
     * @param rowIndex the row to select. It becomes the selection, which is passed to the MainController.
     */
    void selectOneRowAndNotify(int rowIndex) noexcept;

    /**
     * @return true if the index is selected.
     * @param index the index.
     */
    bool isIndexSelected(int index) const noexcept;

    /** The user wants to rename the selected item. If invalid, nothing happens (but this should have been checked before). */
    void onUserWantsToRenameSelected() const noexcept;
    /** The user wants to delete the selected item. If invalid, nothing happens (but this should have been checked before). */
    void onUserWantsToDeleteSelected() noexcept;
    /** The user wants to "enter" the selected item (by pressing "enter" for example). Nothing happens if there are more than one selected. */
    void onUserWantsToEnterSelected() noexcept;

    /**
     * The user wants to add a new item.
     * @param index the index if present. If absent, it will be put at the end.
     */
    void onUserWantsToAddAt(OptionalInt index) noexcept;

    /** @return true if the selection has at least one item, and item 0 does not belong it. */
    bool isSelectedPresentAndValid() const noexcept;

    /** Copies the selection to the clipboard. */
    void copy() const noexcept;
    /** Cuts the selection to the clipboard. */
    void cut() noexcept;
    /** Pastes the selection from the clipboard, after the selection. "After", so that the user can paste something when there is nothing, and at the end. */
    void pasteAfter() noexcept;

    /** Duplicates the selected items after the last selected. */
    void duplicateItemsAfter() noexcept;

    /**
     * Call this after a notification of a change in the selection (typically called after a click on an item, the MC is called, and
     * it calls its observers). It will take care of selecting the item uniquely or not.
     * @param selectedIndex the index to select.
     * @param forceSingleSelection true to cancel the possibly multiple selection. Useful when deleting items, for example.
     */
    void onSelectedItemChanged(int selectedIndex, bool forceSingleSelection) noexcept;

    /** Asks the subclass whether it can perform the operation. By default, Load and save may. */
    virtual bool canOperationBePerformedSub(int commandId) const noexcept;

    /** @return true if the item 0 is selected. */
    bool is0Selected() const noexcept;
    /** @return true if the only one item is selected. */
    bool isOneOnlySelected() const noexcept;

    /** @return the main controller. */
    MainController& getMainController() const noexcept;

private:
    /**
     * Double-notifies: first, the selected item, then all the selection. The selectedIndexes member must have been updated.
     * @param selectedItemIndex the selected item index. Should be valid.
     */
    void notifyNewSelection(int selectedItemIndex) noexcept;

    /**
     * Pastes given items, stored in a single root items. "After", so that the user can paste something when there is nothing, and at the end.
     * If successful, and if there is only one item, the user is asked to rename the item.
     */
    void pasteAfterAndPromptRename(const juce::XmlElement& items) noexcept;

    /**
     * Prompts the user for an item rename.
     * @param itemIndex the index of the item to rename.
     */
    void promptRenameItem(int itemIndex) const noexcept;

    MainController& mainController;
    /** The possible selected indexes. It is useful for multiple selections. It does NOT indicate which one is really selected. The Main Controller knows that. */
    std::set<int> selectedIndexes;
    std::unique_ptr<ListView> listView;                 // The ListView.
    std::set<int> dragnDropSelectedIndexes;             // Stored for the case of d'n'd. A bit hackish, but works.
};

}   // namespace arkostracker
