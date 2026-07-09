#pragma once

#include "ExpressionAction.h"

namespace arkostracker
{

/** Action to rename an Expression. */
class RenameExpression final : public ExpressionAction
{
public:
    RenameExpression(SongController& songController, bool isArpeggio, Id expressionId, juce::String newName) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies of the change. */
    void notify() const noexcept;

    const Id expressionId;
    juce::String newName;
    juce::String oldName;
};

}   // namespace arkostracker
