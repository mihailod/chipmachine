#pragma once

#include "ExpressionAction.h"

namespace arkostracker
{

class SongController;

/**
 * Sets the cell of an Expression. If the cell index is out of bounds, cells are added with the new value.
 * Nothing happens if the value is the same, or if the cell index is <0 (asserts).
 */
class SetExpressionCell final : public ExpressionAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param id the expression Id. Must be valid.
     * @param cellIndex the cell index (>=0).
     * @param value the new value.
     */
    SetExpressionCell(SongController& songController, bool isArpeggio, Id id, int cellIndex, int value) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the change in the Expression.
     * @param mustRefreshPast true if the cells after the target one must also be refreshed (because added/removed for example).
     */
    void notifyChange(bool mustRefreshPast) const noexcept;

    const Id id;
    const int cellIndex;
    const int newValue;

    int originalEnd;
    int newEnd;
    int originalLength;
    int originalValue;
    int createdBarCount;                  // Useful to shrink the expression in the redo.
    int refreshIndexStart;
};

}   // namespace arkostracker
