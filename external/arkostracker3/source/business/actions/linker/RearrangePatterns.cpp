#include "RearrangePatterns.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

RearrangePatterns::RearrangePatterns(SongController& pSongController, Id pSubsongId, const bool pDeleteUnusedPatterns) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        newSubsongId(),
        deleteUnusedPatterns(pDeleteUnusedPatterns),
        oldPositions(),
        oldPatterns()
{
}

bool RearrangePatterns::perform()
{
    auto modified = false;

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Copies all the data for the Undo.
        oldPositions = subsong.getPositions(true);
        oldPatterns = subsong.getPatterns();
        const auto oldPatternCount = oldPatterns.size();

        std::unordered_map<int, int> oldPatternIndexToNewPatternIndex;
        std::vector<Position> newPositions;
        std::vector<Pattern> newPatterns;

        // Parses the positions, remapping the patterns in increasing order.
        for (const auto& position : oldPositions) {
            const auto patternIndex = position.getPatternIndex();
            // Does this pattern exist in the remapping?
            if (const auto it = oldPatternIndexToNewPatternIndex.find(patternIndex); it == oldPatternIndexToNewPatternIndex.cend()) {
                // It does not exist. Then it must be remapped.
                const auto newPatternIndex = static_cast<int>(oldPatternIndexToNewPatternIndex.size());
                oldPatternIndexToNewPatternIndex.insert({ patternIndex, newPatternIndex });

                const auto newPosition = position.withPatternIndex(newPatternIndex);
                newPositions.push_back(newPosition);

                newPatterns.push_back(oldPatterns.at(static_cast<size_t>(patternIndex)));
            } else {
                // The pattern exists. The position can simply use it.
                const auto newPosition = position.withPatternIndex(it->second);
                newPositions.push_back(newPosition);
            }
        }

        // Appends the unused patterns, those that weren't used by any Position.
        if (!deleteUnusedPatterns) {
            for (size_t patternIndex = 0U; patternIndex < oldPatternCount; ++patternIndex) {
                if (const auto it = oldPatternIndexToNewPatternIndex.find(static_cast<int>(patternIndex)); it == oldPatternIndexToNewPatternIndex.cend()) {
                    newPatterns.push_back(oldPatterns.at(patternIndex));
                }
            }

            jassert(oldPositions.size() == newPositions.size());
            jassert(oldPatterns.size() == newPatterns.size());
        }

        // Nothing more to do if all was already well arranged.
        if ((oldPositions != newPositions) || (oldPatterns != newPatterns)) {
            modified = true;

            // Replaces the old data.
            subsong.setPositions(newPositions);
            subsong.setPatterns(newPatterns);
        }
    });

    if (modified) {
        notify(subsongId);
    }

    return modified;
}

bool RearrangePatterns::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Replaces the new data.
        subsong.setPositions(oldPositions);
        subsong.setPatterns(oldPatterns);
    });

    notify(subsongId);

    return true;
}

void RearrangePatterns::notify(const Id& subsongIdToShow) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongIdToShow, { });
    });
}

}   // namespace arkostracker
