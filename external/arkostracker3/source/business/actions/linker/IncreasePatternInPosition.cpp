#include "IncreasePatternInPosition.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

IncreasePatternInPosition::IncreasePatternInPosition(SongController& pSongController, Id pSubsongId, const int pPositionIndex, const int pStep) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        step(pStep),
        oldPosition()
{
}

bool IncreasePatternInPosition::perform()
{
    const auto song = songController.getSong();
    auto modified = false;

    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        oldPosition = std::make_unique<Position>(subsong.getPosition(positionIndex));

        const auto oldPatternIndex = oldPosition->getPatternIndex();
        const auto patternCount = subsong.getPatternCount();
        // Makes sure the new pattern index is within bounds!
        const auto newPatternIndex = NumberUtil::correctNumber(oldPatternIndex + step, 0, patternCount - 1);

        if (newPatternIndex != oldPatternIndex) {
            const auto newPosition = oldPosition->withPatternIndex(newPatternIndex);
            subsong.setPosition(positionIndex, newPosition);

            modified = true;
        }
    });

    // Notifies, if needed.
    if (modified) {
        notifyListeners();
    }

    return modified;
}

bool IncreasePatternInPosition::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Sets the old data.
        subsong.setPosition(positionIndex, *oldPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void IncreasePatternInPosition::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, positionIndex, LinkerObserver::What::patternIndex);
    });
}

}   // namespace arkostracker
