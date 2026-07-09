#pragma once

#include "ExpressionAction.h"

namespace arkostracker
{

/** Deletes an Expression Cell, if possible. Cannot delete if the length is 1. */
class DeleteExpressionCell final : public ExpressionAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param id the expression Id. Must be valid.
     * @param cellIndex the cell index (>=0).
     */
    DeleteExpressionCell(SongController& songController, bool isArpeggio, Id id, int cellIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the change in the Expression. */
    void notifyChange() const noexcept;

    const Id id;
    const int cellIndex;
    Loop originalLoop;
    int originalValue;
};

}   // namespace arkostracker
