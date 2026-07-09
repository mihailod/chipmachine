#include "CreatePattern.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/SetUtil.h"

namespace arkostracker 
{

CreatePattern::CreatePattern(SongController& pSongController, Id pSubsongId, const int pSourcePositionIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        sourcePositionIndex(pSourcePositionIndex)
{
}

bool CreatePattern::perform()
{
    const auto song = songController.getSong();

    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        const auto sourcePosition = subsong.getPosition(sourcePositionIndex);
        const auto& sourcePattern = subsong.getPatternRef(sourcePositionIndex);

        const auto channelCount = subsong.getChannelCount();

        // Creates new empty Tracks.
        std::vector<int> newTrackIndexes;
        newTrackIndexes.reserve(static_cast<size_t>(channelCount));
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            const auto newTrackIndex = subsong.addTrack(Track());
            newTrackIndexes.push_back(newTrackIndex);
        }
        // Creates new empty Special Tracks.
        const auto speedTrackIndex = subsong.addSpecialTrack(true, SpecialTrack());
        const auto eventTrackIndex = subsong.addSpecialTrack(false, SpecialTrack());

        // We can create the Pattern.
        const Pattern newPattern(newTrackIndexes, speedTrackIndex, eventTrackIndex, sourcePattern.getArgbColor());
        const auto newPatternIndex = subsong.addPattern(newPattern);

        // Then the Position using it. The Marker name is removed, more user friendly.
        const auto newPosition = sourcePosition.withPatternIndex(newPatternIndex).withMarkerName(juce::String());
        subsong.addPosition(sourcePositionIndex, true, newPosition);
    });

    // Notifies. The new Position is highlighted.
    notifyListeners(sourcePositionIndex + 1);

    return true;
}

bool CreatePattern::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        const auto channelCount = subsong.getChannelCount();

        // Removes the added Position.
        subsong.removePositions(sourcePositionIndex, true, 1);
        // Removes the added Patterns, Tracks/event/speed Tracks. They are at the end!
        subsong.removeLastPatterns(1);
        subsong.removeLastTracks(channelCount);
        subsong.removeLastSpecialTracks(true, 1);
        subsong.removeLastSpecialTracks(false, 1);
    });

    // Notifies.
    notifyListeners(sourcePositionIndex);

    return true;
}

void CreatePattern::notifyListeners(int highlightedPositionIndex) const noexcept
{
    // FIXME TO IMPROVE. Required to refresh the PV.
    const Location location(subsongId, highlightedPositionIndex);
    songController.setCurrentLocation(location, false);
    // Goto but no refresh on PV, the fix above is required.
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) {
        observer->onLinkerPositionsInvalidated(subsongId, { highlightedPositionIndex });
    });
}

}   // namespace arkostracker
