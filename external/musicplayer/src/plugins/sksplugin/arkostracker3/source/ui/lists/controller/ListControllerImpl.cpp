#include "ListControllerImpl.h"

#include "../../../controllers/MainController.h"
#include "../../../utils/SetUtil.h"
#include "../../keyboard/CommandIds.h"
#include "../../utils/Clipboard.h"
#include "../view/ListViewNoOp.h"

namespace arkostracker 
{

ListControllerImpl::ListControllerImpl(MainController& pMainController) noexcept :
        mainController(pMainController),
        selectedIndexes(),
        listView(std::unique_ptr<ListViewNoOp>()),          // For now, a no-op implementation.
        dragnDropSelectedIndexes()
{
}

void ListControllerImpl::getKeyboardFocus() noexcept
{
    listView->grabKeyboardFocus();
}


// ParentViewLifeCycleAware method implementations.
// =================================================

void ListControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    // We can now create a "real" View.
    listView = createListView();

    // Initializes the list, any is selected?
    listView->updateListContent();
    updateShownSelectedRows();

    parentView.addAndMakeVisible(*listView);
}

void ListControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    listView->setBounds(startX, startY, newWidth, newHeight);
}

void ListControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
    // Nothing to do.
}

void ListControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to a no-op View.
    listView = std::make_unique<ListViewNoOp>();
}


// ListItemListener method implementations.
// ==========================================

void ListControllerImpl::onListItemClicked(const int rowIndex, const juce::ModifierKeys modifierKeys, const bool isLeftMouseDown)
{
    const auto isLeftMouseUp = !isLeftMouseDown;

    // The tricky part is:
    // - Allow d'n'd on one row, selected or not.
    // - Left click on several selected rows should select only the clicked one.
    // - Drag'n'drop on several selected rows should be allowed.
    // However, only doing that is not great because clicking on an already selected rank will not unselect the rest!
    // If selects all, then you cannot select one row only!
    if (!modifierKeys.isAnyModifierKeyDown()) {
        // If no shift, click on an already selected row: do nothing. This will allow d'n'd on several rows.
        if (isLeftMouseDown && isIndexSelected(rowIndex)) {
            // This is called when a d'n'd is getting started. Stores the current indexes. A bit hackish, but
            // this selection gets cleared before they can be treated by onDragListItem, so it's easier to manage it this way.
            dragnDropSelectedIndexes = selectedIndexes;
            return;
        }

        selectOneRowAndNotify(rowIndex);
    } else if (isLeftMouseDown) {
        // Shift case.
        // JUCE wants us to call a special method to manage the multiple selection.
        listView->selectRowsBasedOnModifierKeys(rowIndex, modifierKeys, isLeftMouseUp);     // Special JUCE method.

        manageSelectionChanged();
    }
}

void ListControllerImpl::manageSelectionChanged() noexcept
{
    // Gets the selected rows. If nothing is selected, selects 0.
    const auto selection = listView->getSelectedRows();
    selectedIndexes = selection.getSelectedIndexesAsSet();

    const auto selectedItemIndexOptional = selection.getLastlySelectedIndex();
    const auto selectedItemIndex = selectedItemIndexOptional.isAbsent() ? 0 : selectedItemIndexOptional.getValue();

    notifyNewSelection(selectedItemIndex);
}

void ListControllerImpl::notifyNewSelection(const int selectedItemIndex) noexcept
{
    const auto selectedItemId = getIdFromIndex(selectedItemIndex);
    if (selectedItemId.isAbsent()) {
        jassertfalse;       // ID not found? Abnormal.
        return;
    }
    // Notifies the selected item.
    notifyNewSelectedId(selectedItemId.getValueRef());

    // Also sends the subclass the selected indexes/last selected index to send an observation (useful for PV toolbox).
    // Removes the selected index to put it at the end.
    auto newSelectedIndexesSet = selectedIndexes;
    newSelectedIndexesSet.erase(selectedItemIndex);
    auto newSelectedIndexes = SetUtil::setToVector(newSelectedIndexesSet);
    newSelectedIndexes.push_back(selectedItemIndex);
    notifySelection(newSelectedIndexes);
}

void ListControllerImpl::onListItemDoubleClicked(int /*rowIndex*/)
{
    onUserWantsToEnterSelected();        // We consider the double click on selected only one item. If not, nothing will happen anyway.
}

void ListControllerImpl::onListItemRightClicked(const int rowIndex)
{
    // If the row was not selected, selects it only.
    if (!isIndexSelected(rowIndex)) {
        selectOneRowAndNotify(rowIndex);
    }

    showPopUpMenu();
}

void ListControllerImpl::onDragListItem(int /*sourceRowIndex*/, const int destinationRowIndex)
{
    // Cannot insert before 0, forbidden.
    if (destinationRowIndex == 0) {
        return;
    }
    // We use the previously selected drag'n'drop selection (if present, else use the selection). A bit hackish, but simple enough.
    moveItems(dragnDropSelectedIndexes.empty() ? selectedIndexes : dragnDropSelectedIndexes, destinationRowIndex);

    dragnDropSelectedIndexes.clear();
}

void ListControllerImpl::onDeleteKeyPressed()
{
    onUserWantsToDeleteSelected();
}

void ListControllerImpl::onReturnKeyPressed()
{
    onUserWantsToEnterSelected();
}

void ListControllerImpl::onListItemSelectionChanged()
{
    // This is only called when the keyboard is used in the ListBox!
    // But can be with shift, ctrl, or not.
    manageSelectionChanged();
}

// ======================================

void ListControllerImpl::showPopUpMenu() noexcept
{
    auto& commandManager = mainController.getCommandManager();

    juce::PopupMenu popupMenu;
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::copy, juce::translate("Copy"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::cut, juce::translate("Cut"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::paste, juce::translate("Paste"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, listInsertItemAfter, juce::translate("Create new after"));
    popupMenu.addCommandItem(&commandManager, listAddItem, juce::translate("Create at the bottom"));
    popupMenu.addCommandItem(&commandManager, listDuplicateItems, juce::translate("Duplicate"));
    popupMenu.addCommandItem(&commandManager, listRenameItem, juce::translate("Rename"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::del, juce::translate("Delete"));

    // The subclass may want to add its own entries.
    putAdditionalPopUpMenuItems(popupMenu, commandManager);

    popupMenu.show();
}

void ListControllerImpl::updateShownSelectedRows() const noexcept
{
    // Not efficient... Shouldn't be a problem.
    juce::SparseSet<int> set;
    for (const auto& index : selectedIndexes) {
        const juce::Range range(index, index + 1);
        set.addRange(range);
    }
    listView->showSelection(set);
}

void ListControllerImpl::selectOneRowAndNotify(const int rowIndex) noexcept
{
    juce::SparseSet<int> newSelection;
    newSelection.addRange(juce::Range(rowIndex, rowIndex + 1));

    selectedIndexes.clear();
    selectedIndexes.insert(rowIndex);

    listView->showSelection(newSelection);

    // Notifies.
    notifyNewSelection(rowIndex);
}

bool ListControllerImpl::isIndexSelected(const int index) const noexcept
{
    return (selectedIndexes.find(index) != selectedIndexes.cend());
}

void ListControllerImpl::onUserWantsToRenameSelected() const noexcept
{
    if ((selectedIndexes.size() != 1) || !isSelectedPresentAndValid()) {
        jassertfalse;           // Invalid selected, should have been checked before.
        return;
    }

    const auto index = *selectedIndexes.cbegin();
    promptRenameItem(index);
}

void ListControllerImpl::onUserWantsToDeleteSelected() noexcept
{
    if (!isSelectedPresentAndValid()) {
        jassertfalse;   // Invalid selected, should have been checked before, BUT the Del key is hardcoded in JUCE, so this can happen when using the keyboard.
        return;
    }

    onUserWantsToDelete(selectedIndexes);
}

void ListControllerImpl::onUserWantsToEnterSelected() noexcept
{
    if (!isOneOnlySelected()) {
        return;
    }

    enterSelectedItem();
}

void ListControllerImpl::onUserWantsToAddAt(const OptionalInt optionalIndex) noexcept
{
    // Uses the index if given (+1 to put AFTER), else put at the end.
    const auto index = optionalIndex.isPresent() ? (optionalIndex.getValue() + 1) : getItemCount();
    // Don't do anything if invalid.
    if (index < 0) {
        jassertfalse;           // Should have been checked earlier.
        return;
    }

    // Asks the user to prompt for the name.
    promptToAddNewItemAndPerform(index);
}

bool ListControllerImpl::isSelectedPresentAndValid() const noexcept
{
    return (!selectedIndexes.empty() && (selectedIndexes.find(0) == selectedIndexes.cend()));
}

void ListControllerImpl::copy() const noexcept
{
    // Don't do anything if the selection is not valid.
    if (!isSelectedPresentAndValid()) {
        return;
    }

    // Gets the XML node of the items to copy.
    const auto xmlElement = getItemsToCopy(selectedIndexes);

    // Stores it to the clipboard.
    if (xmlElement != nullptr) {
        Clipboard::storeXmlNode(xmlElement.get());
    }
}

void ListControllerImpl::cut() noexcept
{
    // Don't do anything if the selection is not valid.
    if (!isSelectedPresentAndValid()) {
        return;
    }
    copy();

    // Deletes the selection.
    deleteItems(selectedIndexes);
}

void ListControllerImpl::pasteAfter() noexcept
{
    // Anything in the clipboard?
    const auto xmlElement = Clipboard::retrieveXmlNode();
    if (xmlElement == nullptr) {
        return;
    }

    pasteAfterAndPromptRename(*xmlElement);
}

void ListControllerImpl::duplicateItemsAfter() noexcept
{
    // Gets the XML node of the items to copy. Simpler to use the XML already existing code...
    const auto xmlElement = getItemsToCopy(selectedIndexes);
    if (xmlElement == nullptr) {
        return;
    }

    pasteAfterAndPromptRename(*xmlElement);
}

void ListControllerImpl::pasteAfterAndPromptRename(const juce::XmlElement& items) noexcept
{
    // Where to paste?
    const auto selectedIndexOptional = getSelectedIndex();
    int selectedIndex;          // NOLINT(*-init-variables)
    if (selectedIndexOptional.isPresent()) {
        selectedIndex = selectedIndexOptional.getValue() + 1;       // We go PAST it.
    } else {
        selectedIndex = getItemCount();
    }

    // Performs the paste. We don't get the items here so as not to manage item types here. Let the subclass do it.
    const auto itemIds = deserializeXmlItemsAndPaste(items, selectedIndex);

    // Asks for a rename, but only if one item.
    if (itemIds.size() == 1) {
        promptRenameItem(selectedIndex);
    }
}

void ListControllerImpl::promptRenameItem(const int itemIndex) const noexcept
{
    listView->promptRenameItem(itemIndex);
}

void ListControllerImpl::updateList() const noexcept
{
    listView->updateListContent();
}

void ListControllerImpl::updateItem(const int itemIndex) const noexcept
{
    listView->updateRow(itemIndex);
}

void ListControllerImpl::onSelectedItemChanged(const int selectedIndex, const bool forceSingleSelection) noexcept
{
    // Don't do anything if the row is already selected. This is needed for multiple selection,
    // but not done when performing an action such as Delete (in which case forceSingleSelection is true).
    if (!forceSingleSelection && isIndexSelected(selectedIndex)) {
        return;
    }

    // The row wasn't selected. Selects it only.
    selectedIndexes.clear();
    selectedIndexes.insert(selectedIndex);
    updateShownSelectedRows();
}

// ListViewListener method implementations.
// =========================================================

void ListControllerImpl::onUserWantsToDelete(const std::set<int>& originalIndexesToDelete) noexcept
{
    // Expression 0 is removed, we cannot delete it.
    auto indexesToDelete = originalIndexesToDelete;
    indexesToDelete.erase(0);

    if (indexesToDelete.empty()) {
        return;
    }

    // We don't prompt the user, too boring for him.
    deleteItems(indexesToDelete);
}

void ListControllerImpl::onUserConfirmedRename(const int itemIndex, const juce::String& newName) noexcept
{
    if (itemIndex <= 0) {
        jassertfalse;           // Should have been checked before!
        return;
    }

    renameItem(itemIndex, newName);
}

void ListControllerImpl::onUserWantsToAdd() noexcept
{
    // Adds at the end.
    onUserWantsToAddAt(OptionalInt());
}

/*void ListControllerImpl::onUserConfirmedAdd(int itemIndex, const juce::String& name) noexcept
{
    if (itemIndex <= 0) {
        jassertfalse;           // Should have been checked before!
        return;
    }

    addNewItem(itemIndex, name);
}*/

void ListControllerImpl::getAllCommands(juce::Array<juce::CommandID>& commands) noexcept
{
    getListCommands().getAllCommands(commands);
}

void ListControllerImpl::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result) noexcept
{
    getListCommands().getCommandInfo(commandID, result);     // This will check what operation is allowed, thanks to the object passed to its constructor.
}

bool ListControllerImpl::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept
{
    auto success = true;
    switch (info.commandID) {
        case juce::StandardApplicationCommandIDs::copy:
            copy();
            break;
        case juce::StandardApplicationCommandIDs::cut:
            cut();
            break;
        case juce::StandardApplicationCommandIDs::paste:
            pasteAfter();
            break;
        case juce::StandardApplicationCommandIDs::del:
            onUserWantsToDeleteSelected();
            break;

        case listRenameItem:
            onUserWantsToRenameSelected();
            break;
        case listAddItem:
            onUserWantsToAddAt(OptionalInt());       // Puts at the end.
            break;
        case listDuplicateItems:
            duplicateItemsAfter();
            break;
        case listInsertItemAfter: {
            const auto lastlySelectedIndexOptional = getSelectedIndex();
            onUserWantsToAddAt(lastlySelectedIndexOptional);
            break;
        }
        default:
            // No command? Asks the subclass.
            success = performSubCommand(info);
            if (!success) {
                jassertfalse;       // Command not managed. Rather abnormal.
            }
            break;
    }
    return success;
}

bool ListControllerImpl::is0Selected() const noexcept
{
    return (selectedIndexes.find(0) != selectedIndexes.cend());
}

bool ListControllerImpl::isOneOnlySelected() const noexcept
{
    return (selectedIndexes.size() == 1);
}

MainController& ListControllerImpl::getMainController() const noexcept
{
    return mainController;
}

// OperationValidityProvider method implementations.
// ================================================

bool ListControllerImpl::canOperationBePerformed(const int commandId) const noexcept
{
    const auto zeroSelected = is0Selected();
    const auto empty = selectedIndexes.empty();

    switch (commandId) {
        case juce::StandardApplicationCommandIDs::copy: [[fallthrough]];
        case juce::StandardApplicationCommandIDs::cut: [[fallthrough]];
        case juce::StandardApplicationCommandIDs::del: [[fallthrough]];
        case listDuplicateItems:
            return (!zeroSelected && !empty);
        case juce::StandardApplicationCommandIDs::paste:
        case listInsertItemAfter:
            return true;
        case listRenameItem:
            return (!zeroSelected && isOneOnlySelected());
        case listAddItem:
            return true;
        default:
            // Lets the subclass manage this command.
            return canOperationBePerformedSub(commandId);
    }
}

bool ListControllerImpl::canOperationBePerformedSub(const int commandId) const noexcept
{
    switch (commandId) {
        case listLoadItem:
            return isOneOnlySelected();
        case listSaveItem:
            return !is0Selected() && isOneOnlySelected();
        default:
            jassertfalse;       // Operation not managed??
            return false;
    }
}


}   // namespace arkostracker

