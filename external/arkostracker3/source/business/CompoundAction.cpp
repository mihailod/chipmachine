#include "CompoundAction.h"

namespace arkostracker
{

void CompoundAction::addAction(std::unique_ptr<UndoableAction> action) noexcept
{
    actions.push_back(std::move(action));
}

bool CompoundAction::perform()
{
    successfulActions.clear();

    // Performs all the Actions.
    for (const auto& action : actions) {
        const auto actionSuccess = action->perform();
        // Stores only the ones that were successful, for an undo.
        // Stores them at the front to undo them in reverse order.
        if (actionSuccess) {
            successfulActions.insert(successfulActions.begin(), action.get());
        }
    }

    return !successfulActions.empty();
}

bool CompoundAction::undo()
{
    auto success = true;        // All Undo should be a success!

    // Performs all the Actions, stored in reverse order.
    for (auto* action : successfulActions) {
        success = success && action->undo();
    }

    jassert(success);

    return success;
}

int CompoundAction::getSizeInUnits()
{
    // Makes the sum of the "size in units" of all the Actions.
    auto sum = 0;
    for (const auto& action : actions) {
        sum += action->getSizeInUnits();
    }

    return sum;
}

bool CompoundAction::isEmpty() const noexcept
{
    return actions.empty();
}

} // namespace arkostracker
