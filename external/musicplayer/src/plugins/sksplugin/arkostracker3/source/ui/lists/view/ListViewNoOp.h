#pragma once

#include "ListView.h"

namespace arkostracker 
{

/** A ListView that does nothing. */
class ListViewNoOp final : public ListView
{
public:
    void updateListContent() noexcept override;
    void updateRow(int rowIndex) noexcept override;
    void selectRowsBasedOnModifierKeys(int rowIndex, juce::ModifierKeys modifierKeys, bool isMouseUpEvent) noexcept override;
    void showSelection(const juce::SparseSet<int>& selectedRows) noexcept override;
    SelectionInList getSelectedRows() const noexcept override;
    void promptRenameItem(int index) noexcept override;
    juce::String getPromptRenameTitle() const noexcept override;
};


}   // namespace arkostracker

