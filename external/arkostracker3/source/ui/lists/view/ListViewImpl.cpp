#include "ListViewImpl.h"

#include <BinaryData.h>

#include "../../components/dialogs/TextFieldDialog.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "ListViewListener.h"

namespace arkostracker 
{

ListViewImpl::ListViewImpl(ListViewListener& pListViewListener, ListItemListener& pListItemListener,
                           DataProvider& pDataProvider, std::unique_ptr<juce::ListBoxModel> pModel) noexcept :
        listViewListener(pListViewListener),
        listItemListener(pListItemListener),
        dataProvider(pDataProvider),
        listBoxModel(std::move(pModel)),
        listBox(listBoxModel.get(), false),       // CTRL+A not authorized.
        addButton(BinaryData::IconNew_png, BinaryData::IconNew_pngSize, juce::translate("Add a new item at the bottom"),
                  [&](int, bool, bool) { onAddButtonClicked(); }),
        modalDialog()
{
    listBox.setMultipleSelectionEnabled(true);
    listBox.setRowHeight(LookAndFeelConstants::listItemHeights);

    addAndMakeVisible(listBox);
    addAndMakeVisible(addButton);
}


// ListView method implementations.
// =================================================

void ListViewImpl::updateListContent() noexcept
{
    listBox.updateContent();
}

void ListViewImpl::updateRow(const int rowIndex) noexcept
{
    listBox.repaintRow(rowIndex);
}

void ListViewImpl::selectRowsBasedOnModifierKeys(const int rowIndex, const juce::ModifierKeys modifierKeys, const bool isMouseUpEvent) noexcept
{
    listBox.selectRowsBasedOnModifierKeys(rowIndex, modifierKeys, isMouseUpEvent);
}

void ListViewImpl::showSelection(const juce::SparseSet<int>& selectedRows) noexcept
{
    listBox.setSelectedRows(selectedRows, juce::NotificationType::dontSendNotification);
    // Scrolls to the last (by convention) selected items.
    if (!selectedRows.isEmpty()) {
        // Hack... I don't know why by using scrollToEnsureRowIsOnscreen only, the item 1 is shown cut on startup.
        // So if the selection is below a few items, scrolls to the top.
        // If changing this, makes sure the selected items goes back well when selecting another panel (arp, pitch) and going back to instrument.
        if (const auto viewedItemIndex = selectedRows[selectedRows.size() - 1]; viewedItemIndex < 4) {
            listBox.setVerticalPosition(0);
        } else {
            listBox.scrollToEnsureRowIsOnscreen(viewedItemIndex);
        }
    }
}

SelectionInList ListViewImpl::getSelectedRows() const noexcept
{
    // Any last row selected?
    const auto lastRowSelected = listBox.getLastRowSelected();
    const auto lastRowOptional = (lastRowSelected < 0) ? OptionalInt() : OptionalInt(lastRowSelected);

    return { listBox.getSelectedRows(), lastRowOptional };
}

void ListViewImpl::promptRenameItem(const int index) noexcept
{
    jassert(index > 0);
    jassert(modalDialog == nullptr);        // Already a dialog present? Abnormal.

    auto title = getPromptRenameTitle();
    auto oldName = dataProvider.getItemTitle(index);
    modalDialog = std::make_unique<TextFieldDialog>(
            title, juce::translate("Enter the new name"), oldName,
            [&] (const juce::String& newName) { onDialogRenameTextValidated(newName); },
            [&] { onDialogCancelled(); },
            true);
}


// Component method implementations.
// ================================

void ListViewImpl::resized()
{
    const auto bounds = getLocalBounds();

    constexpr auto buttonsWidth = 40;
    constexpr auto buttonsHeight = 25;
    constexpr auto margins = 5;

    listBox.setBounds(bounds);

    const auto addButtonX = bounds.getWidth() - (margins * 7) - buttonsWidth;     // A bit far from the right border to allow clicking on the color icon.
    const auto addButtonY = bounds.getHeight() - margins - buttonsHeight;
    addButton.setBounds(addButtonX, addButtonY, buttonsWidth, buttonsHeight);
}

void ListViewImpl::focusOfChildComponentChanged(const FocusChangeType /*cause*/)
{
    setBackgroundColorFromFocus();
}

void ListViewImpl::lookAndFeelChanged()
{
    // Needed to repaint the background when changing theme.
    setBackgroundColorFromFocus();
}


// ListItemListener method implementations.
// ==========================================

void ListViewImpl::onListItemClicked(const int rowIndex, const juce::ModifierKeys modifierKeys, const bool isLeftMouseDown)
{
    listItemListener.onListItemClicked(rowIndex, modifierKeys, isLeftMouseDown);
}

void ListViewImpl::onListItemDoubleClicked(const int rowIndex)
{
    listItemListener.onListItemDoubleClicked(rowIndex);
}

void ListViewImpl::onListItemRightClicked(const int rowIndex)
{
    listItemListener.onListItemRightClicked(rowIndex);
}

void ListViewImpl::onDragListItem(const int sourceRowIndex, const int destinationRowIndex)
{
    listItemListener.onDragListItem(sourceRowIndex, destinationRowIndex);
}

void ListViewImpl::onDeleteKeyPressed()
{
    listItemListener.onDeleteKeyPressed();
}

void ListViewImpl::onReturnKeyPressed()
{
    listItemListener.onReturnKeyPressed();
}

void ListViewImpl::onListItemSelectionChanged()
{
    listItemListener.onListItemSelectionChanged();
}


// ApplicationCommandTarget method implementations.
// ================================================

juce::ApplicationCommandTarget* ListViewImpl::getNextCommandTarget()
{
    return findFirstTargetParentComponent();        // Command not found here, goes up the UI hierarchy.
}

void ListViewImpl::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    listViewListener.getAllCommands(commands);
}

void ListViewImpl::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    listViewListener.getCommandInfo(commandID, result);
}

bool ListViewImpl::perform(const ApplicationCommandTarget::InvocationInfo& info)
{
    return listViewListener.perform(info);
}


// =======================================================

void ListViewImpl::onWantToRename(const int expressionIndex, const juce::String& newName) const noexcept
{
    listViewListener.onUserConfirmedRename(expressionIndex, newName);
}

void ListViewImpl::onDialogRenameTextValidated(const juce::String& text) noexcept
{
    // There must be only one selection.
    const auto selected = dataProvider.getSelectedIndex();
    if (selected.isAbsent()) {
        jassertfalse;       // The UI should have prevented this!
        return;
    }

    onWantToRename(selected.getValue(), text);
    modalDialog.reset();            // Done AFTER the text is used.
}

void ListViewImpl::onDialogCancelled() noexcept
{
    modalDialog.reset();
}

void ListViewImpl::onAddButtonClicked() const noexcept
{
    listViewListener.onUserWantsToAdd();
}

void ListViewImpl::setBackgroundColorFromFocus() noexcept
{
    const auto hasFocus = hasKeyboardFocus(true);

    // When there are not a lot of lines, the rest is filled with the background color. However, it does not change in account the focus,
    // so we do it ourselves.
    const auto backgroundColor = findColour(static_cast<int>(
        hasFocus ? LookAndFeelConstants::Colors::panelBackgroundFocused : LookAndFeelConstants::Colors::panelBackgroundUnfocused));

    listBox.setColour(juce::ListBox::ColourIds::backgroundColourId, backgroundColor);
}

}   // namespace arkostracker
