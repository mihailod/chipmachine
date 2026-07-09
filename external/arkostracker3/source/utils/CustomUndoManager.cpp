#include "CustomUndoManager.h"

namespace arkostracker 
{

CustomUndoManager::CustomUndoManager(int maxNumberOfUnitsToKeep, int minimumTransactionsToKeep) noexcept :
        undoManager(maxNumberOfUnitsToKeep, minimumTransactionsToKeep),
        indexOfNonModified(),
        currentIndex()
{
}

void CustomUndoManager::clearUndoHistory()
{
    undoManager.clearUndoHistory();
    indexOfNonModified = 0;
    currentIndex = 0;
}

bool CustomUndoManager::perform(juce::UndoableAction* action)
{
    const auto done = undoManager.perform(action);
    if (done) {
        ++currentIndex;
    }
    return done;
}

void CustomUndoManager::beginNewTransaction(const juce::String& actionName)
{
    undoManager.beginNewTransaction(actionName);
}

bool CustomUndoManager::canUndo() const
{
    return undoManager.canUndo();
}

bool CustomUndoManager::undo()
{
    const auto done = undoManager.undo();
    if (done) {
        --currentIndex;
    }
    return done;
}

bool CustomUndoManager::canRedo() const
{
    return undoManager.canRedo();
}

bool CustomUndoManager::redo()
{
    const auto done = undoManager.redo();
    if (done) {
        ++currentIndex;
    }
    return done;
}

juce::String CustomUndoManager::getUndoDescription() const
{
    return undoManager.getUndoDescription();
}

juce::String CustomUndoManager::getRedoDescription() const
{
    return undoManager.getRedoDescription();
}

void CustomUndoManager::setCurrentStateAsNonModified() noexcept
{
    indexOfNonModified = currentIndex;
}

bool CustomUndoManager::isCurrentStateModified() const noexcept
{
    return (indexOfNonModified != currentIndex);
}


}   // namespace arkostracker

