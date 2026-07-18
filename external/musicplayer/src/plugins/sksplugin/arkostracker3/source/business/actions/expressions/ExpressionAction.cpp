#include "ExpressionAction.h"

#include "../../../song/ExpressionHandler.h"
#include "../../../controllers/SongController.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../song/tool/browser/CellBrowser.h"
#include "../../song/tool/ChangeExpressionsOrdering.h"

namespace arkostracker
{

ExpressionAction::ExpressionAction(SongController& pSongController, bool pIsArpeggio) noexcept:
        songController(pSongController),
        song(pSongController.getSong()),
        isArpeggio(pIsArpeggio),
        modifiedCells()
{
}

ExpressionHandler& ExpressionAction::getExpressionHandler() noexcept
{
    return song->getExpressionHandler(isArpeggio);
}

const ExpressionHandler& ExpressionAction::getExpressionHandler() const noexcept
{
    return song->getExpressionHandler(isArpeggio);
}

bool ExpressionAction::isAnArpeggio() const noexcept
{
    return isArpeggio;
}

void ExpressionAction::setSelectedExpression(int newIndex, bool forceSingleSelection) const noexcept
{
    auto& mainController = songController.getMainController();
    auto expressionId = songController.getExpressionId(isArpeggio, newIndex);
    // After a Delete, the index doesn't exist anymore if it was the last one.
    // It is more user-friendly to select the last one instead of nothing.
    if (expressionId.isAbsent()) {
        expressionId = songController.getLastExpressionId(isArpeggio);
    }
    
    mainController.setSelectedExpressionId(isArpeggio, expressionId, forceSingleSelection);
}

void ExpressionAction::notifyGlobalChange() const noexcept
{
    // The PV will also observe this.
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) {
        observer->onExpressionsInvalidated();
    });
}


// ----------------------------------------------------

bool ExpressionAction::performOnMapper(Remapper<Expression>& remapper) noexcept
{
    auto& expressionHandler = getExpressionHandler();

    // First, changes the Expressions themselves.
    expressionHandler.performOnExpressions([&] (std::vector<std::unique_ptr<Expression>>& expressions) noexcept {
        remapper.applyOnVector(expressions);
    });

    // Then, changes the mapping in the Tracks.
    const auto& mapping = remapper.getMapping();
    modifiedCells = ChangeExpressionsOrdering::changeExpressionsInSong(isArpeggio, *song, mapping);

    notifyGlobalChange();

    return true;
}

bool ExpressionAction::undoOnMapper(Remapper<Expression>& remapper) noexcept
{
    // Changes the ordering.
    auto& expressionHandler = getExpressionHandler();
    expressionHandler.performOnExpressions([&] (std::vector<std::unique_ptr<Expression>>& expressions) noexcept {
        remapper.undoOnVector(expressions);
    });

    // Changes the Cells.
    const CellBrowser cellBrowser(*song);
    cellBrowser.writeCells(modifiedCells);

    modifiedCells.clear();

    notifyGlobalChange();

    return true;
}

}   // namespace arkostracker
