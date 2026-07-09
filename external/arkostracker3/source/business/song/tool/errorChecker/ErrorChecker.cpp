#include "ErrorChecker.h"

#include "../../../../song/Song.h"
#include "../../../../ui/patternViewer/controller/EffectErrorHelper.h"
#include "../../cells/CellEffectsChecker.h"
#include "../browser/CellBrowser.h"

namespace arkostracker
{

std::vector<std::pair<CellLocationInPosition, juce::String>> ErrorChecker::findErrors(const Song& song) noexcept
{
    std::vector<std::pair<CellLocationInPosition, juce::String>> result;

    const auto subsongIds = song.getSubsongIds();
    for (const auto& subsongId : subsongIds) {
        // Browses all the cells from each position.
        CellBrowser::browseCellsFromPositions(song, subsongId, true, [&](const CellLocationInPosition& cellLocationInPosition, const Cell& cell) {
            const auto indexToEffectError = CellEffectsChecker::checkEffects(cell.getEffects(), cell.isNote(), cell.isInstrument());

            // Any error? If yes, stores the displayable message and where it happened.
            for (const auto& [_, effectError] : indexToEffectError) {
                const auto displayableError = EffectErrorHelper::effectErrorToDisplayableString(effectError);

                result.emplace_back(cellLocationInPosition, displayableError);
            }

            return false;
        });
    }

    return result;
}

}   // namespace arkostracker
