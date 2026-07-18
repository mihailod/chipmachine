#pragma once

#include "ExpressionAction.h"
#include "../lists/DeleteItems.h"

class SongController;

namespace arkostracker
{

/** Action to delete Expressions. This also shifts the others and modifies the song. */
class DeleteExpressions final : public ExpressionAction,
                                DeleteItems<Expression>
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param indexesToDelete the indexes of the Expressions to delete.
     */
    DeleteExpressions(SongController& songController, bool isArpeggio, std::set<int> indexesToDelete) noexcept;

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
