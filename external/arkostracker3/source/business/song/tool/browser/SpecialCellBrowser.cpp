#include "SpecialCellBrowser.h"

#include "../../../../song/Song.h"
#include "../../../../utils/SetUtil.h"

namespace arkostracker 
{

void SpecialCellBrowser::browseConstSongSpecialCells(const Song& song, const bool speedTrack, const bool stopAtLoopEnd,
                                                     const std::function<void(const SpecialCell&)>& onCellRead) noexcept
{
    // Performs for each Subsong.
    for (const auto& subsongId : song.getSubsongIds()) {
        browseConstSubsongSpecialCells(song, subsongId, speedTrack, stopAtLoopEnd, onCellRead);
    }
}

void SpecialCellBrowser::browseConstSubsongSpecialCells(const Song& song, const Id& subsongId, const bool speedTrack, const bool stopAtLoopEnd,
                                                 const std::function<void(const SpecialCell&)>& onCellRead) noexcept
{
    const auto specialTrackIndexes = getAllUsedSpecialTrackIndexes(song, subsongId, speedTrack, stopAtLoopEnd);

    for (const auto specialTrackIndex : specialTrackIndexes) {
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            const auto& specialTrack = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack);

            // Browses each Cell.
            for (auto cellIndex = 0; cellIndex < TrackConstants::maximumCapacity; ++cellIndex) {
                const auto& cell = specialTrack.getCellRefConst(cellIndex);

                // Applies the operation to it.
                onCellRead(cell);
            }
        });
    }
}

std::unordered_set<int> SpecialCellBrowser::getAllUsedSpecialTrackIndexes(const Song& song, const Id& subsongId, const bool speedTrack, const bool stopAtLoopEnd) noexcept
{
    auto lastPositionIndex = 0;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        lastPositionIndex = stopAtLoopEnd ? subsong.getEndPosition() : subsong.getLength() - 1;
    });

    // Cumulates all the SpecialTrack indexes.
    std::unordered_set<int> specialTrackIndexes;
    for (auto positionIndex = 0; positionIndex <= lastPositionIndex; ++positionIndex) {
        song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
            const auto& pattern = subsong.getPatternRef(positionIndex);
            const auto specialTrackIndex = pattern.getCurrentSpecialTrackIndex(speedTrack);
            specialTrackIndexes.insert(specialTrackIndex);
        });
    }

    return specialTrackIndexes;
}

std::vector<SpecialCellAndLocation> SpecialCellBrowser::browseSpecialCells(Song& song, const bool speedTrack, const bool stopAtLoopEnd,
                                                                    const std::function<std::unique_ptr<SpecialCell>(const SpecialCell&)>& operationOnCell) noexcept
{
    std::vector<SpecialCellAndLocation> totalModifiedCells;

    // Performs for each Subsong.
    for (const auto& subsongId : song.getSubsongIds()) {
        auto modifiedCells = browseSpecialCells(song, subsongId, speedTrack, stopAtLoopEnd, operationOnCell);

        // Agglomerates the sub-result.
        totalModifiedCells.insert(totalModifiedCells.cend(), modifiedCells.begin(), modifiedCells.end());
    }

    return totalModifiedCells;
}

std::vector<SpecialCellAndLocation> SpecialCellBrowser::browseSpecialCells(Song &song, const Id &subsongId, bool speedTrack, const bool stopAtLoopEnd,
                                                                           const std::function<std::unique_ptr<SpecialCell>(const SpecialCell &)> &operationOnCell) noexcept
{
    std::vector<SpecialCellAndLocation> totalModifiedCells;

    // How many Tracks?
    const auto specialTrackIndexes = getAllUsedSpecialTrackIndexes(song, subsongId, speedTrack, stopAtLoopEnd);

    // Browses each Track.
    for (const auto specialTrackIndex : specialTrackIndexes) {
        song.performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
            auto& specialTrack = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack);

            // Browses each Cell.
            for (auto cellIndex = 0; cellIndex < TrackConstants::maximumCapacity; ++cellIndex) {
                const auto& cell = specialTrack.getCellRefConst(cellIndex);

                // Applies the operation to it.
                const auto newCellIfModified = operationOnCell(cell);

                if (newCellIfModified != nullptr) {
                    // Stores the old cells if wanted (useful for Undo/Redo for example).
                    // This is done BEFORE writing because the Cell above will be destroyed.
                    totalModifiedCells.emplace_back(speedTrack, cell, SpecialCellLocationInTrack(speedTrack, subsongId, specialTrackIndex, cellIndex));

                    // This Cell must replace the current one.
                    specialTrack.setCell(cellIndex, *newCellIfModified);
                }
            }
        });
    }

    return totalModifiedCells;
}

void SpecialCellBrowser::writeSpecialCells(Song& song, const std::vector<SpecialCellAndLocation>& cellsAndLocations) noexcept
{
    // Splits into various collections, per Subsong.
    std::unordered_map<Id, std::vector<SpecialCellAndLocation>> subsongToCells;
    // Puts the cellData in the right Collection of the Set.
    for (const auto& cellAndLocation : cellsAndLocations) {
        const auto& location = cellAndLocation.getCellLocation();
        const auto& subsongId = location.getSubsongId();
        SetUtil::addItemToMapOfVectors(subsongToCells, subsongId, cellAndLocation);
    }

    // Browses through each Subsong, and puts the cells.
    for (const auto& [subsongId, subsongCells] : subsongToCells) {
        const auto& localSubsongCells = subsongCells;       // Captured structured bindings not allowed in <C++20.
        const auto& localSubsongId = subsongId;
        song.performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
            for (const auto& cellData : localSubsongCells) {
                const auto& cellLocation = cellData.getCellLocation();
                const auto speedTrack = cellLocation.isSpeedTrack();
                jassert(localSubsongId == cellLocation.getSubsongId()); (void)localSubsongId;       // Else, the sorting has failed!

                subsong.setSpecialCellFromTrackIndex(speedTrack, cellLocation.getTrackIndex(), cellLocation.getLineIndex(), cellData.getCell());
            }
        });
    }
}

}   // namespace arkostracker
