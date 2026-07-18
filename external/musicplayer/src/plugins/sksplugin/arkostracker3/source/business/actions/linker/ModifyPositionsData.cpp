#include "ModifyPositionsData.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

ModifyPositionsData::ModifyPositionsData(SongController& pSongController, Id pSubsongId, const std::set<int>& pPositionIndexes, const OptionalInt pNewHeight,
    const OptionalValue<juce::uint32> pNewPatternColorArgb) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndexes(pPositionIndexes),
        newHeight(pNewHeight),
        newPatternColorArgb(pNewPatternColorArgb),
        oldPositionIndexToPosition(),
        oldPatternIndexToPattern()
{
}

bool ModifyPositionsData::perform()
{
    // Nothing to do?
    if (positionIndexes.empty()) {
        jassertfalse;       // Shouldn't happen.
        return false;
    }
    if (newHeight.isAbsent() && newPatternColorArgb.isAbsent()) {
        return false;
    }

    oldPositionIndexToPosition.clear();
    oldPatternIndexToPattern.clear();

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        std::unordered_set<int> patternIndexes;

        // Browses the positions to change their height (if wanted), and also to get the pattern indexes.
        for (const auto positionIndex : positionIndexes) {
            auto position = subsong.getPosition(positionIndex);

            oldPositionIndexToPosition.insert({ positionIndex, position });     // Stores for the undo.

            if (newPatternColorArgb.isPresent()) {
                patternIndexes.insert(position.getPatternIndex());
            }

            if (newHeight.isPresent()) {
                position.setHeight(newHeight.getValue());
                subsong.setPosition(positionIndex, position);
            }
        }

        // Changes the pattern colors. The indexes are empty of the user doesn't want to do that.
        for (const auto patternIndex : patternIndexes) {
            const auto originalPattern = subsong.getPatternFromIndex(patternIndex);
            oldPatternIndexToPattern.insert({ patternIndex, originalPattern });     // Stores for the undo.

            const auto newPattern = originalPattern.withColor(newPatternColorArgb.getValue());
            subsong.setPattern(patternIndex, newPattern);
        }
    });

    notifyListeners();

    return true;
}

bool ModifyPositionsData::undo()
{
    if (oldPositionIndexToPosition.empty() && oldPatternIndexToPattern.empty()) {
        jassertfalse;       // Nothing stored?
        return false;
    }

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Puts back the positions, then the patterns.
        for (const auto& [positionIndex, position] : oldPositionIndexToPosition) {
            subsong.setPosition(positionIndex, position);
        }

        for (const auto& [patternIndex, pattern] : oldPatternIndexToPattern) {
            subsong.setPattern(patternIndex, pattern);
        }
    });

    notifyListeners();

    return true;
}

void ModifyPositionsData::notifyListeners() const noexcept
{
    const std::unordered_set indexes(positionIndexes.cbegin(), positionIndexes.cend());

    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsChanged(subsongId, indexes, LinkerObserver::What::positionHeight | LinkerObserver::What::patternMetadata);
    });
}

}   // namespace arkostracker
