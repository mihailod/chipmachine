#pragma once

#include "ExpressionAction.h"

namespace arkostracker
{

/** Sets the metadata (loop, speed, etc.) of an Expression. Does NOT concern the cells. */
class SetExpressionCellMetadata final : public ExpressionAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the ID of the expression. It must exist.
     * @param newLoopStart if present, the new loop start. Must be valid.
     * @param newEnd if present, the new end. Must be valid.
     * @param newSpeed if present, the new speed.
     * @param newShift if present, the new shift.
     */
    SetExpressionCellMetadata(SongController& songController, bool isArpeggio, Id expressionId, OptionalInt newLoopStart, OptionalInt newEnd,
                              OptionalInt newSpeed, OptionalInt newShift) noexcept;

    bool perform() override;
    bool undo() override;

private:
    const Id expressionId;
    const OptionalInt loopStart;
    const OptionalInt end;
    const OptionalInt speed;
    const OptionalInt shift;
    Loop oldLoop;
    int oldSpeed;
    int oldShift;

    /** Notifies the listeners of a change in the Cell. */
    void notify() const noexcept;
};

}   // namespace arkostracker
