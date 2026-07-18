#pragma once

#include "../../../ui/containerArranger/PanelType.h"
#include "../../utils/ParentViewLifeCycleAware.h"
#include "../ListCommands.h"
#include "../view/ListView.h"
#include "DataProvider.h"

namespace arkostracker
{

/** An abstract controller for a List Controller (for Expressions, Instruments, etc.). */
class ListController : public ParentViewLifeCycleAware,
                       public DataProvider
{
public:
    /** @return an instance of the ListView. */
    virtual std::unique_ptr<ListView> createListView() noexcept = 0;

    /**
     * @return the ID from the given index, if found.
     * @param index the index. May be invalid.
     */
    virtual OptionalId getIdFromIndex(int index) const noexcept = 0;

    /**
     * Asks the subclass to move the items. Checks have been done before by this class.
     * @param indexesToMove the indexes to move.
     * @param targetIndex the target index, without taking care if the indexes to move.
     */
    virtual void moveItems(const std::set<int>& indexesToMove, int targetIndex) noexcept = 0;

    /**
     * Asks the subclass to delete the items. Checks have been done before by this class.
     * @param indexesToDelete the indexes to delete.
     */
    virtual void deleteItems(const std::set<int>& indexesToDelete) noexcept = 0;

    /**
     * Asks the subclass to rename the item. Checks have been done before by this class.
     * @param itemIndex the index to rename.
     * @param newName the new name.
     */
    virtual void renameItem(int itemIndex, const juce::String& newName) noexcept = 0;

    /**
     * Asks the subclass to add a new item. Checks have been done before by this class.
     * This could show a prompt after the item to add, then will perform the action.
     * @param itemIndex the index where to insert.
     */
    virtual void promptToAddNewItemAndPerform(int itemIndex) noexcept = 0;

    /** "Enter" into the selected item. There must be only one. This is called when double-clicking, or enter. */
    virtual void enterSelectedItem() noexcept = 0;

    /**
     * Notifies the (Song/Main) controller about a selection that has changed. This class will receive a notification.
     * @param newSelectedItemId the new selected index.
     */
    virtual void notifyNewSelectedId(Id newSelectedItemId) noexcept = 0;

    /**
     * Notifies about a selection that has changed.
     * @param selectedIndexes all the selected indexes, the last item being the selected.
     */
    virtual void notifySelection(const std::vector<int>& selectedIndexes) noexcept = 0;

    /**
     * @return a Xml Element of the items that has been copied. Empty if none can be.
     * @param indexesToCopy the indexes to copy.
     */
    virtual std::unique_ptr<juce::XmlElement> getItemsToCopy(const std::set<int>& indexesToCopy) const noexcept = 0;

    /**
     * Asks the subclass to deserialize the XML items and paste them. The subclass must check the items are valid,
     * and return empty if any is not.
     * @param serializedItems the items.
     * @param targetIndex the target index.
     * @return the IDs of the new items, in order. Empty if fails.
     */
    virtual std::vector<Id> deserializeXmlItemsAndPaste(const juce::XmlElement& serializedItems, int targetIndex) noexcept = 0;

    /** @return the ListCommand to use for this class. */
    virtual ListCommands& getListCommands() noexcept = 0;

    /**
     * Subclasses may want to add more items in the pop-up menu. They will be added at the bottom. The default implementation does nothing.
     * @param popupMenu the pop-up menu.
     * @param commandManager the Command Manager to use.
     */
    virtual void putAdditionalPopUpMenuItems(juce::PopupMenu& popupMenu, juce::ApplicationCommandManager& commandManager) noexcept
    {
        // The default implementation does nothing.
        (void)popupMenu;
        (void)commandManager;
    }

    /** Asks the subclass to perform its own command, if any. */
    virtual bool performSubCommand(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept
    {
        // The default implementation does nothing.
        (void)info;
        return false;
    }

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;

    /** @return the panel to open if an item is double-clicked for example. */
    virtual PanelType getRelatedPanelToOpen() const noexcept = 0;
};

}   // namespace arkostracker

