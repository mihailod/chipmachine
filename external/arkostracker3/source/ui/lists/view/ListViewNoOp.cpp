#include "ListViewNoOp.h"

namespace arkostracker 
{

void ListViewNoOp::updateListContent() noexcept
{
}

void ListViewNoOp::updateRow(int /*rowIndex*/) noexcept
{
}

void ListViewNoOp::selectRowsBasedOnModifierKeys(int /*rowIndex*/, juce::ModifierKeys /*modifierKeys*/, bool /*isMouseUpEvent*/) noexcept
{
}

void ListViewNoOp::showSelection(const juce::SparseSet<int>& /*selectedRows*/) noexcept
{
}

SelectionInList ListViewNoOp::getSelectedRows() const noexcept
{
    return { juce::SparseSet<int>(), OptionalInt() };
}

void ListViewNoOp::promptRenameItem(int /*index*/) noexcept
{
}

juce::String ListViewNoOp::getPromptRenameTitle() const noexcept
{
    return {};
}

}   // namespace arkostracker
