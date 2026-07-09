#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace arkostracker 
{

/**
 * An Undo Manager which manages observers to indicate whether the song is modified or not.
 * It wraps JUCE's Undo Manager via composition.
 */
class CustomUndoManager
{
public:
    /**
     * Constructor.
     * @param maxNumberOfUnitsToKeep each UndoableAction object returns a value to indicate how much storage it takes up (UndoableAction::getSizeInUnits()), so this
     * lets you specify the maximum total number of units that the undo manager is allowed to keep in memory before letting the older actions
     * drop off the end of the list.
     * @param minimumTransactionsToKeep this specifies the minimum number of transactions that will be kept, even if this involves exceeding
     * the amount of space specified in maxNumberOfUnitsToKeep
     */
    explicit CustomUndoManager(int maxNumberOfUnitsToKeep = 30000, int minimumTransactionsToKeep = 30) noexcept;

    /** Deletes all stored actions in the list. */
    void clearUndoHistory();

    /**
     * Performs an action and adds it to the undo history list.
     * @param action the action to perform - this object will be deleted by the UndoManager when no longer needed
     * @returns true if the command succeeds - see UndoableAction::perform
     * @see beginNewTransaction
     */
    bool perform(juce::UndoableAction* action);

    /**
     * Starts a new group of actions that together will be treated as a single transaction.
     * All actions that are passed to the perform() method between calls to this
     * method are grouped together and undone/redone together by a single call to
     * undo() or redo().
     * @param actionName a description of the transaction that is about to be performed
     */
    void beginNewTransaction(const juce::String& actionName);

    /**
     * @return true if there's at least one action in the list to undo.
     * @see getUndoDescription, undo, canRedo
     */
    bool canUndo() const;

    /**
     * Tries to roll-back the last transaction.
     * @return true if the transaction can be undone, and false if it fails, or if there aren't any transactions to undo
     * @see undoCurrentTransactionOnly
     */
    bool undo();

    /**
     * Return true if there's at least one action in the list to redo.
     * @see getRedoDescription, redo, canUndo
     */
    bool canRedo() const;

    /**
     * Tries to redo the last transaction that was undone.
     * @return true if the transaction can be redone, and false if it fails, or if there aren't any transactions to redo
     */
    bool redo();

    /**
     * @return the name of the transaction that will be rolled-back when undo() is called.
     * @see undo, canUndo, getUndoDescriptions
     */
    juce::String getUndoDescription() const;

    /**
     * @return the name of the transaction that will be redone when redo() is called.
     * @see redo, canRedo, getRedoDescriptions
     */
    juce::String getRedoDescription() const;

    /** Marks the song as saved. It does not call any observer, only refresh the internal state. */
    void setCurrentStateAsNonModified() noexcept;

    /** @return true if the Undo state is "modified". Useful to know if the song has been modified or not. */
    bool isCurrentStateModified() const noexcept;

private:
    juce::UndoManager undoManager;              // The underlying undo manager.
    int indexOfNonModified;                     // The index of the action that marks a "not modified here".
    int currentIndex;
};


}   // namespace arkostracker

