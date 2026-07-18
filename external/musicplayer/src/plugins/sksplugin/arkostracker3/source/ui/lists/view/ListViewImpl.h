#pragma once

#include <memory>

#include "../../components/ButtonWithImage.h"
#include "../../components/CustomListBox.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/ListItemListener.h"
#include "ListView.h"
#include "item/ListModel.h"

namespace arkostracker 
{

class DataProvider;
class ListViewListener;

/** Partial implementation of a ListView. */
class ListViewImpl : public ListView,
                     public ListItemListener,
                     public juce::ApplicationCommandTarget
{
public:
    /**
     * Constructor.
     * @param listener the listener of the event of this View.
     * @param listItemListener the listener of the event of the items.
     * @param dataProvider object that can return some data about the items.
     * @param model the model of the items. Must not be null!
     */
    ListViewImpl(ListViewListener& listener, ListItemListener& listItemListener, DataProvider& dataProvider,
                 std::unique_ptr<juce::ListBoxModel> model) noexcept;

    // ListView method implementations.
    // =================================================
    void updateListContent() noexcept override;
    void updateRow(int rowIndex) noexcept override;
    void selectRowsBasedOnModifierKeys(int rowIndex, juce::ModifierKeys modifierKeys, bool isMouseUpEvent) noexcept override;
    void showSelection(const juce::SparseSet<int>& selectedRows) noexcept override;
    SelectionInList getSelectedRows() const noexcept override;
    void promptRenameItem(int index) noexcept override;

    // Component method implementations.
    // ===================================
    void resized() override;
    void focusOfChildComponentChanged(FocusChangeType cause) override;
    void lookAndFeelChanged() override;

    // ListItemListener method implementations.
    // ==========================================
    void onListItemClicked(int rowIndex, juce::ModifierKeys modifierKeys, bool isLeftMouseDown) override;
    void onListItemDoubleClicked(int rowIndex) override;
    void onListItemRightClicked(int rowIndex) override;
    void onDragListItem(int sourceRowIndex, int destinationRowIndex) override;
    void onDeleteKeyPressed() override;
    void onReturnKeyPressed() override;
    void onListItemSelectionChanged() override;

    // ApplicationCommandTarget method implementations.
    // ================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    /**
     * Called when the user wants to rename an expression (either via right-click or Button for example).
     * @param expressionIndex the index. As a convenience, if <=0, nothing happens.
     * @param newName the new name.
     */
    void onWantToRename(int expressionIndex, const juce::String& newName) const noexcept;

    /** Called from the Add Dialog when its text is validated. */
    //void onDialogAddTextValidated(const juce::String& text) noexcept;
    /** Called from the Rename Dialog when its text is validated. */
    void onDialogRenameTextValidated(const juce::String& text) noexcept;
    /** Called from any Dialog when it is cancelled. */
    void onDialogCancelled() noexcept;

    /** Called when the Add Button is clicked. */
    void onAddButtonClicked() const noexcept;

    /** Sets the background color, according to the current focus. */
    void setBackgroundColorFromFocus() noexcept;

    ListViewListener& listViewListener;                     // Listener of the specific events of this class.
    ListItemListener& listItemListener;                     // For the client to be notified of the events.
    DataProvider& dataProvider;                             // Provides the data to display.

    std::unique_ptr<juce::ListBoxModel> listBoxModel;
    CustomListBox listBox;                                  // Where all the items are displayed.
    ButtonWithImage addButton;

    std::unique_ptr<ModalDialog> modalDialog;
};

}   // namespace arkostracker
