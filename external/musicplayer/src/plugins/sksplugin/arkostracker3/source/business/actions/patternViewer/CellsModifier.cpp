#include "CellsModifier.h"

#include "../../../controllers/SongController.h"
#include "../../../utils/CollectionUtil.h"

namespace arkostracker 
{

CellsModifier::CellsModifier(SongController& pSongController, SelectedData pSelectedData) noexcept :
        songController(pSongController),
        selectedData(std::move(pSelectedData)),
        oldCellsAndLocations(),
        oldSpecialCellsAndLocations()
{
}


// UndoableAction method implementations.
// =========================================

bool CellsModifier::perform()
{
    jassert(oldCellsAndLocations.empty());
    jassert(oldSpecialCellsAndLocations.empty());
    // Only a security, shouldn't happen.
    oldCellsAndLocations.clear();
    oldSpecialCellsAndLocations.clear();

    const auto& trackToCaptureFlag = selectedData.getTrackToCaptureFlag();
    const auto includeSpeedTrack = selectedData.getIncludeSpeedTrack();
    const auto includeEventTrack = selectedData.getIncludeEventTrack();

    const auto selectedChannelCount = static_cast<int>(trackToCaptureFlag.size());
    if ((selectedChannelCount == 0) && !includeSpeedTrack && !includeEventTrack) {
        jassertfalse;           // No location to perform on? Strange.
        return false;
    }

    const auto& startLocation = selectedData.getStartLocation();
    const auto& endLocationOptional = selectedData.getEndLocation();

    // What is the end location? If not given, set it to the end of the position.
    const auto endLocation = endLocationOptional.isPresent()
            ? endLocationOptional.getValue()
            : startLocation.withLine(TrackConstants::lastPossibleIndex);

    // Sanity checks: the subsong of the start/end must be same, the position must be following.
    const auto subsongId = startLocation.getSubsongId();
    if (subsongId != endLocation.getSubsongId()) {
        jassertfalse;       // Not the same Subsong! Abnormal!
        return false;
    }
    const auto startPositionIndex = startLocation.getPosition();
    const auto endPositionIndex = endLocation.getPosition();
    if (startPositionIndex > endPositionIndex) {
        jassertfalse;       // Start position superior to end, abnormal!
        return false;
    }

    const auto startChannelIndex = selectedData.getStartChannelIndex();
    auto modified = false;
    const auto endChannelIndex = startChannelIndex + selectedChannelCount - 1;      // May be out of bounds!

    std::unordered_set<int> alreadyBrowsedTrackIndexes;
    std::unordered_set<int> alreadyBrowsedSpeedTrackIndexes;
    std::unordered_set<int> alreadyBrowsedEventTrackIndexes;

    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        const auto channelCount = subsong.getChannelCount();
        // If out of bounds, nothing to do.
        if (startChannelIndex >= channelCount) {
            return;
        }

        // Browses each position.
        for (auto positionIndex = startPositionIndex; positionIndex <= endPositionIndex; ++positionIndex) {
            // Starts line depends on if we are on the start or end position, or in between.
            const auto startLine = (positionIndex == startPositionIndex) ? startLocation.getLine() : 0;
            const auto endLine = (positionIndex == endPositionIndex) ? endLocation.getLine() : TrackConstants::lastPossibleIndex;
            jassert(startLine <= endLine);

            // Browses each channels.
            for (auto channelIndex = startChannelIndex, channelIndexFrom0 = 0; (channelIndex < channelCount) && (channelIndex <= endChannelIndex);
                 ++channelIndex, ++channelIndexFrom0) {

                // Don't browse this channel if the Track is already done! Because linking possible.
                const auto trackIndex = subsong.getPatternRef(positionIndex).getCurrentTrackIndex(channelIndex);
                if (CollectionUtil::contains(alreadyBrowsedTrackIndexes, trackIndex)) {
                    continue;
                }
                alreadyBrowsedTrackIndexes.insert(trackIndex);

                const auto& captureFlags = trackToCaptureFlag.at(static_cast<size_t>(channelIndexFrom0));

                // Browses the line of the position.
                for (auto cellIndex = startLine; cellIndex <= endLine; ++cellIndex) {
                    // Gets the cell and processes it.
                    // Also indicates the capture flags. It is up to the client to ignore it or take it in account.
                    const auto readCell = subsong.getCell(positionIndex, cellIndex, channelIndex);
                    const auto newCell = performOnCell(readCell, captureFlags);

                    // Any change?
                    if (newCell != nullptr) {
                        subsong.setCell(positionIndex, cellIndex, channelIndex, *newCell);
                        // Stores the old Cell for the Undo.
                        const auto cellAndLocation = std::pair(CellLocationInPosition(subsongId, positionIndex, cellIndex, channelIndex), readCell);
                        oldCellsAndLocations.push_back(cellAndLocation);

                        modified = true;
                    }
                }
            }

            // The same for the Special Tracks.
            if (includeSpeedTrack) {
                modified |= browseSelectionOnSpecialTrack(subsong, alreadyBrowsedSpeedTrackIndexes, true, subsongId, positionIndex, startLine, endLine);
            }
            if (includeEventTrack) {
                modified |= browseSelectionOnSpecialTrack(subsong, alreadyBrowsedEventTrackIndexes, false, subsongId, positionIndex, startLine, endLine);
            }
        }
    });

    if (modified) {
        notifyListeners();
    }

    return modified;
}

bool CellsModifier::browseSelectionOnSpecialTrack(Subsong& subsong, std::unordered_set<int>& alreadyBrowsedSpecialTrackIndexesToFill,
    const bool isSpeedTrack, const Id& subsongId, const int positionIndex, const int startLine, const int endLine) noexcept
{
    // Don't browse this SpecialTrack if already done! Because linking possible.
    const auto specialTrackIndex = subsong.getPatternRef(positionIndex).getCurrentSpecialTrackIndex(isSpeedTrack);
    if (CollectionUtil::contains(alreadyBrowsedSpecialTrackIndexesToFill, specialTrackIndex)) {
        return false;
    }
    alreadyBrowsedSpecialTrackIndexesToFill.insert(specialTrackIndex);

    auto modified = false;
    // Browses the line of the position.
    for (auto cellIndex = startLine; cellIndex <= endLine; ++cellIndex) {
        // Gets the cell and processes it.
        const auto readSpecialCell = subsong.getSpecialCell(isSpeedTrack, positionIndex, cellIndex);
        const auto newSpecialCell = performOnSpecialCell(isSpeedTrack, readSpecialCell);
        // Any change?
        if (newSpecialCell != nullptr) {
            subsong.setSpecialCell(isSpeedTrack, positionIndex, cellIndex, *newSpecialCell);
            const auto specialCellAndLocation = std::pair(SpecialCellLocationInPosition(isSpeedTrack, subsongId, positionIndex, cellIndex), readSpecialCell);
            // Stores the old Cell for the Undo.
            oldSpecialCellsAndLocations.push_back(specialCellAndLocation);

            modified = true;
        }
    }

    return modified;
}

bool CellsModifier::undo()
{
    // Puts the stored Cells "back".
    const auto& startLocation = selectedData.getStartLocation();
    const auto subsongId = startLocation.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        for (const auto&[cellLocationInPosition, cell] : oldCellsAndLocations) {
            jassert(cellLocationInPosition.getSubsongId() == subsongId);      // Terribly abnormal!
            subsong.setCell(cellLocationInPosition.getPositionIndex(), cellLocationInPosition.getLineIndex(), cellLocationInPosition.getChannelIndex(), cell);
        }

        // The same for the special Tracks.
        for (const auto&[location, specialCell] : oldSpecialCellsAndLocations) {
            jassert(location.getSubsongId() == subsongId);      // Terribly abnormal!
            subsong.setSpecialCell(location.isSpeedTrack(), location.getPositionIndex(), location.getLineIndex(), specialCell);
        }
    });

    oldCellsAndLocations.clear();
    oldSpecialCellsAndLocations.clear();

    notifyListeners();

    return true;
}

void CellsModifier::notifyListeners() const noexcept
{
    const auto& startLocation = selectedData.getStartLocation();

    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(startLocation.getSubsongId());
    });
}

std::unique_ptr<SpecialCell> CellsModifier::performOnSpecialCell(bool /*isSpeedTrack*/, const SpecialCell& /*inputSpecialCell*/) const noexcept
{
    return nullptr;
}

const SelectedData& CellsModifier::getSelectedData() const noexcept
{
    return selectedData;
}

}   // namespace arkostracker
