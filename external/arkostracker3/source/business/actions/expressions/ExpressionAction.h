#pragma once

#include <memory>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Remapper.h"
#include "../../../song/Expression.h"
#include "../../song/tool/browser/CellAndLocation.h"

namespace arkostracker
{

class Song;
class SongController;
class ExpressionHandler;

/** A base Action for Expressions. */
class ExpressionAction : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to get the Song, and to notify the changes.
     * @param isArpeggio true for Arpeggio, false for Pitches.
     */
    explicit ExpressionAction(SongController& songController, bool isArpeggio) noexcept;

protected:
    /** @return the object that knows how to handle the Expressions. */
    ExpressionHandler& getExpressionHandler() noexcept;
    /** @return the object that knows how to handle the Expressions. */
    const ExpressionHandler& getExpressionHandler() const noexcept;

    /**
     * Selects an expression. It will be corrected.
     * @param index the index (>0).
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after expressions are deleted, for example.
     */
    void setSelectedExpression(int index, bool forceSingleSelection) const noexcept;

    /** Notifies the observers about global change. */
    void notifyGlobalChange() const noexcept;

    /** Performs the action using the given remapper, which must have been setup before. */
    bool performOnMapper(Remapper<Expression>& remapper) noexcept;
    /** Undoes the action using the given remapper, which must have been setup before. */
    bool undoOnMapper(Remapper<Expression>& remapper) noexcept;

    /** @return true if Arpeggio, false if Pitch. */
    bool isAnArpeggio() const noexcept;

    SongController& songController;
    std::shared_ptr<Song> song;
    const bool isArpeggio;
    std::vector<CellAndLocation> modifiedCells;                    // Filled for undo.
};

}   // namespace arkostracker
