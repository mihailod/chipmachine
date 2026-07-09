#pragma once

#include "ExpressionAction.h"

namespace arkostracker
{

class DuplicateExpressionCell final : public ExpressionAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if arpeggio, false if pitch.
     * @param id the expression Id. Must be valid.
     * @param cellIndex the cell index (>=0).
     */
    DuplicateExpressionCell(SongController& songController, bool isArpeggio, Id id, int cellIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the change in the Expression. */
    void notifyChange() const noexcept;

    const Id id;
    const int cellIndex;
    Loop originalLoop;
};

}   // namespace arkostracker
