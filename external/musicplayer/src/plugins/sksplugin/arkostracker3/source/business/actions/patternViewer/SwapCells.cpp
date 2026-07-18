#include "SwapCells.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/TrackChangeObserver.h"
#include "../../../song/CellLocationInPosition.h"

namespace arkostracker
{

SwapCells::SwapCells(SongController& pSongController, Id pSubsongId, const int pPositionStartIndex, const int pPositionEndIndex,
    const int pTopLineIndex, const int pBottomLineIndex, const int pFirstChannelIndex, const int pSecondChannelIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionStartIndex(pPositionStartIndex),
        positionEndIndex(pPositionEndIndex),
        topLineIndex(pTopLineIndex),
        bottomLineIndex(pBottomLineIndex),
        firstChannelIndex(pFirstChannelIndex),
        secondChannelIndex(pSecondChannelIndex),
        undoData()
{
}

bool SwapCells::perform()
{
    // Sanity checks.
    if ((bottomLineIndex < topLineIndex) || (firstChannelIndex == secondChannelIndex) || (positionStartIndex > positionEndIndex)) {
        jassertfalse;
        return false;
    }

    undoData.clear();

    songController.performOnSubsong(subsongId, [&](Subsong& subsong) {
        if (const auto channelCount = subsong.getChannelCount(); (firstChannelIndex >= channelCount) || (secondChannelIndex >= channelCount)) {
            jassertfalse;       // Channel index out of bounds!
            return;
        }
        if (const auto subsongLength = subsong.getLength(); (positionStartIndex >= subsongLength) || (positionEndIndex >= subsongLength)) {
            jassertfalse;       // Out of bounds position index!
            return;
        }

        // Don't swap already parsed tracks!
        std::unordered_set<int> parsedTrackIndexes;

        // Swaps the Cells of two tracks of each Position. The selected line is not capped on purpose.
        for (auto positionIndex = positionStartIndex; positionIndex <= positionEndIndex; ++positionIndex) {
            // Stores the tracks, to avoid managing them twice.
            const auto& pattern = subsong.getPatternRef(positionIndex);
            const auto firstTrackIndex = pattern.getCurrentTrackIndex(firstChannelIndex);
            const auto secondTrackIndex = pattern.getCurrentTrackIndex(secondChannelIndex);

            if ((parsedTrackIndexes.find(firstTrackIndex) != parsedTrackIndexes.cend())
                || (parsedTrackIndexes.find(secondTrackIndex) != parsedTrackIndexes.cend())) {
                // One of the track is already parsed.
                continue;
            }

            parsedTrackIndexes.insert(firstTrackIndex);
            parsedTrackIndexes.insert(secondTrackIndex);

            for (auto lineIndex = topLineIndex; lineIndex <= bottomLineIndex; ++lineIndex) {
                const auto firstCell = subsong.getCell(positionIndex, lineIndex, firstChannelIndex);
                const auto secondCell = subsong.getCell(positionIndex, lineIndex, secondChannelIndex);

                // A little optimization, don't swap empty cells. However, make sure they have no data at all (clean).
                if (firstCell.isClean() && secondCell.isClean()) {
                    continue;
                }

                // Writes the swapped cells.
                subsong.setCell(positionIndex, lineIndex, firstChannelIndex, secondCell);
                subsong.setCell(positionIndex, lineIndex, secondChannelIndex, firstCell);

                // Stores the original cell.
                undoData.emplace_back( firstCell, CellLocationInTrack(subsongId, firstTrackIndex, lineIndex) );
                undoData.emplace_back( secondCell, CellLocationInTrack(subsongId, secondTrackIndex, lineIndex) );
            }
        }
    });

    const auto changed = !undoData.empty();
    if (changed) {
        notifyListeners();
    }

    return changed;
}

bool SwapCells::undo()
{
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) {
        for (const auto& cellAndLocation : undoData) {
            const auto& cell = cellAndLocation.getCell();
            const auto& location = cellAndLocation.getCellLocation();
            jassert(subsongId == location.getSubsongId());          // Big trouble!

            subsong.setCellFromTrackIndex(location.getTrackIndex(), location.getLineIndex(), cell);
        }
    });

    notifyListeners();

    return true;
}

void SwapCells::notifyListeners() const noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(subsongId);
    });
}

}   // namespace arkostracker
