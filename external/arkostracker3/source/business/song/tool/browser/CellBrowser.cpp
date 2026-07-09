#include "CellBrowser.h"

#include "../../../../song/Song.h"
#include "../../../../utils/SetUtil.h"
#include "TrackBrowser.h"

namespace arkostracker
{
CellBrowser::CellBrowser(Song& pSong) noexcept : song(pSong)
{
}

std::vector<CellAndLocation> CellBrowser::browseCells(const bool returnModifiedCells, const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept
{
    std::vector<CellAndLocation> totalModifiedCells;

    // Performs for each Subsong.
    for (const auto& subsongId : song.getSubsongIds()) {
        auto modifiedCells = browseCells(returnModifiedCells, subsongId, operationOnCell);

        // Agglomerates the sub-result.
        totalModifiedCells.insert(totalModifiedCells.cend(), modifiedCells.begin(), modifiedCells.end());
    }

    return totalModifiedCells;
}

std::vector<CellAndLocation> CellBrowser::browseCells(const bool returnModifiedCells, const Id& subsongId,
                                                      const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept
{
    std::vector<CellAndLocation> totalModifiedCells;

    // How many Tracks?
    auto trackCount = 0;
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
        trackCount = subsong.getTrackCount();
    });

    // Browses each Track.
    for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        auto modifiedCells = browseCells(returnModifiedCells, subsongId, trackIndex, operationOnCell);

        // Agglomerates the sub-result.
        totalModifiedCells.insert(totalModifiedCells.cend(), modifiedCells.begin(), modifiedCells.end());
    }

    return totalModifiedCells;
}

void CellBrowser::browseSubsongCells(const bool stopAtLoopEnd, const Id& subsongId, const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept
{
    const auto trackIndexes = TrackBrowser::getAllUsedTrackIndexes(song, subsongId, stopAtLoopEnd);

    // Performs the work on each Track.
    for (const auto trackIndex : trackIndexes) {
        (void)browseCells(false, subsongId, trackIndex, operationOnCell);
    }
}

std::vector<CellAndLocation> CellBrowser::browseCells(const bool returnModifiedCells, const Id& subsongId, const int trackIndex,
                                                      const std::function<std::unique_ptr<Cell>(const Cell&)>& operationOnCell) const noexcept
{
    std::vector<CellAndLocation> totalModifiedCells;

    song.performOnSubsong(subsongId, [&](Subsong& subsong) {
        auto& track = subsong.getTrackRefFromIndex(trackIndex);

        // Browses each Cell.
        for (auto cellIndex = 0; cellIndex < TrackConstants::maximumCapacity; ++cellIndex) {
            const auto& cell = track.getCellRefConst(cellIndex);

            // Applies the operation to it.
            const auto newCellIfModified = operationOnCell(cell);

            if (newCellIfModified != nullptr) {
                // Stores the old cells if wanted (useful for Undo/Redo for example).
                // This is done BEFORE writing because the Cell above will be destroyed.
                if (returnModifiedCells) {
                    totalModifiedCells.emplace_back(cell, CellLocationInTrack(subsongId, trackIndex, cellIndex));
                }

                // This Cell must replace the current one.
                track.setCell(cellIndex, *newCellIfModified);
            }
        }
    });

    return totalModifiedCells;
}

void CellBrowser::writeCells(const std::vector<CellAndLocation>& cellsAndLocations) const noexcept
{
    // Splits into various collections, per Subsong.
    std::unordered_map<Id, std::vector<CellAndLocation> > subsongToCells;
    // Puts the cellData in the right Collection of the Set.
    for (const auto& cellAndLocation : cellsAndLocations) {
        const auto& location = cellAndLocation.getCellLocation();
        const auto& subsongId = location.getSubsongId();
        SetUtil::addItemToMapOfVectors(subsongToCells, subsongId, cellAndLocation);
    }

    // Browses through each Subsong, and puts the cells.
    for (const auto& [subsongId, subsongCells] : subsongToCells) {
        const auto& localSubsongCells = subsongCells; // Captured structured bindings not allowed in <C++20.
        const auto& localSubsongId = subsongId;
        song.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
            for (const auto& cellData : localSubsongCells) {
                const auto& cellLocation = cellData.getCellLocation();
                jassert(localSubsongId == cellLocation.getSubsongId());
                (void)localSubsongId; // Else, the sorting has failed!

                subsong.setCellFromTrackIndex(cellLocation.getTrackIndex(), cellLocation.getLineIndex(), cellData.getCell());
            }
        });
    }
}

void CellBrowser::browseConstSongCells(const Song& song, const bool stopAtLoopEnd, const std::function<bool(const Cell&)>& onCellRead) noexcept
{
    // Performs for each Subsong.
    for (const auto& subsongId : song.getSubsongIds()) {
        const auto stopped = browseConstSubsongCells(song, subsongId, stopAtLoopEnd, onCellRead);
        if (stopped) {
            return;
        }
    }
}

bool CellBrowser::browseConstSubsongCells(const Song& song, const Id& subsongId, const bool stopAtLoopEnd, const std::function<bool(const Cell&)>& onCellRead) noexcept
{
    auto stopped = false;
    const auto trackIndexes = TrackBrowser::getAllUsedTrackIndexes(song, subsongId, stopAtLoopEnd);

    for (const auto trackIndex : trackIndexes) {
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            if (stopped) {
                return;
            }
            const auto& track = subsong.getTrackRefFromIndex(trackIndex);

            // Browses each Cell.
            for (auto cellIndex = 0; cellIndex < TrackConstants::maximumCapacity && !stopped; ++cellIndex) {
                const auto& cell = track.getCellRefConst(cellIndex);

                // Applies the operation to it.
                stopped = onCellRead(cell);
            }
        });
    }

    return stopped;
}

std::vector<CellLocationInPosition> CellBrowser::browseCellsFromPositions(const Song& song, const Id& subsongId, const bool stopAtLoopEnd,
                                                    const std::function<bool(const CellLocationInPosition& cellLocationInPosition, const Cell&)>& onCellRead) noexcept
{
    std::vector<CellLocationInPosition> results;

    // How long to browse?
    auto endPosition = 0;
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
        endPosition = stopAtLoopEnd ? subsong.getLoop().getEndIndex() : (subsong.getLength() - 1);
    });

    // Creates a lock for each position.
    for (auto positionIndex = 0; positionIndex <= endPosition; ++positionIndex) {
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
            // Browses each channels.
            const auto& position = subsong.getPositionRef(positionIndex);
            const auto positionHeight = position.getHeight();
            const auto& pattern = subsong.getPatternRef(positionIndex);

            auto channelIndex = 0;
            for (const auto trackIndex : pattern.getCurrentTrackIndexes()) {
                const auto& track = subsong.getTrackRefFromIndex(trackIndex);

                // Browses the line of the position.
                for (auto cellIndex = 0; cellIndex < positionHeight; ++cellIndex) {
                    const auto location = CellLocationInPosition(subsongId, positionIndex, cellIndex, channelIndex);
                    // Gets the cell and processes it.
                    const auto storeCell = onCellRead(location, track.getCellRefConst(cellIndex));

                    if (storeCell) {
                        results.emplace_back(location);
                    }
                }

                ++channelIndex;
            }
        });
    }

    return results;
}

} // namespace arkostracker
