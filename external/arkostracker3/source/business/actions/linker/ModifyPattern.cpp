#include "ModifyPattern.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

ModifyPattern::ModifyPattern(SongController& pSongController, Id pSubsongId, const int pPatternIndex, Pattern pNewPattern) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        patternIndex(pPatternIndex),
        newPattern(std::move(pNewPattern)),
        oldPattern()
{
}

bool ModifyPattern::perform()
{
    const auto song = songController.getSong();
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo.
        oldPattern = std::make_unique<Pattern>(subsong.getPatternFromIndex(patternIndex));
    });

    // No change? Then don't do anything.
    if (*oldPattern == newPattern) {
        return false;
    }

    // Sets the new data.
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setPattern(patternIndex, newPattern);
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool ModifyPattern::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Sets the old data.
        subsong.setPattern(patternIndex, *oldPattern);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void ModifyPattern::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPatternInvalidated(subsongId, patternIndex, LinkerObserver::What::patternMetadata);
    });
}

}   // namespace arkostracker
