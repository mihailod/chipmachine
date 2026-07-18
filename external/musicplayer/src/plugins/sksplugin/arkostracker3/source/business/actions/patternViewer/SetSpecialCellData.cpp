#include "SetSpecialCellData.h"

#include "../../../controllers/SongController.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

SetSpecialCellData::SetSpecialCellData(SongController& pSongController, Location pLocation, bool pIsSpeedTrack, const Digit& pDigit) noexcept :
        songController(pSongController),
        location(std::move(pLocation)),
        isSpeedTrack(pIsSpeedTrack),
        digit(pDigit)
{
}

bool SetSpecialCellData::perform()
{
    const auto position = location.getPosition();
    const auto line = location.getLine();

    songController.performOnSubsong(location.getSubsongId(), [&](Subsong& subsong) noexcept {
        // Saves the SpecialCell for undo, but also gets it to modify it.
        auto readSpecialCell = subsong.getSpecialCell(isSpeedTrack, position, line);
        oldSpecialCell = readSpecialCell;

        // Changes the digit.
        const auto originalValue = readSpecialCell.getValue();
        const auto newValue = NumberUtil::replaceDigit(originalValue, digit.digitIndex, digit.digitValue);
        const auto newSpecialCell = SpecialCell::buildSpecialCell(newValue);
        subsong.setSpecialCell(isSpeedTrack, position, line, newSpecialCell);
    });

    // Notifies the listeners of the change.
    notifyListeners();

    return true;
}

bool SetSpecialCellData::undo()
{
    const auto position = location.getPosition();
    const auto line = location.getLine();

    songController.performOnSubsong(location.getSubsongId(), [&](Subsong& subsong) noexcept {
        // Writes the previously saved cell.
        subsong.setSpecialCell(isSpeedTrack, position, line, oldSpecialCell);
    });

    // Notifies the listeners of the change.
    notifyListeners();

    return true;
}

void SetSpecialCellData::notifyListeners() noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(location.getSubsongId());
    });
}


}   // namespace arkostracker

