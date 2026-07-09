#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace arkostracker
{

/**
 * A list of Undoable Actions, so that they can be performed and undone in one action. For example, transposing several Tracks can
 * consist of several linked Action of transpositions, one for each Track.
 */
class CompoundAction final : public juce::UndoableAction
{
public:
    /** Constructor. */
    CompoundAction() noexcept = default;

    /**
     * Adds an Action.
     * @param action the Action.
     */
    void addAction(std::unique_ptr<UndoableAction> action) noexcept;

    /** @return true if there are no actions. */
    bool isEmpty() const noexcept;

    // UndoableAction method implementations.
    // =============================================================
    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;

private:
    std::vector<std::unique_ptr<UndoableAction>> actions;           // The Actions to perform.
    std::vector<UndoableAction*> successfulActions;
};

}   // namespace arkostracker
