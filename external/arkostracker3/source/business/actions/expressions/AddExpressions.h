#pragma once

#include "ExpressionAction.h"
#include "../../../song/Expression.h"
#include "../lists/AddItems.h"

namespace arkostracker
{

class SongController;

/** Action to add an Expression. This also shifts the others and modifies the song. */
class AddExpressions final : public ExpressionAction,
                             AddItems<Expression>
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param indexWhereToInsert the index where to insert.
     * @param originalSelectedIndexForUndo the selected index, only for Undo (to put the selection back).
     * @param expressions the expressions to insert.
     */
    AddExpressions(SongController& songController, bool isArpeggio, int indexWhereToInsert, int originalSelectedIndexForUndo,
                   std::vector<std::unique_ptr<Expression>> expressions) noexcept;

    bool perform() override;
    bool undo() override;

private:
    // DeleteItems method implementations.
    // =================================================
    int getItemCount() const noexcept override;
    void setSelectedItem(int index, bool forceSingleSelection) noexcept override;
    bool performActionOnMapper(Remapper<Expression>& remapper) noexcept override;
    bool undoActionOnMapper(Remapper<Expression>& remapper) noexcept override;
};

}   // namespace arkostracker
