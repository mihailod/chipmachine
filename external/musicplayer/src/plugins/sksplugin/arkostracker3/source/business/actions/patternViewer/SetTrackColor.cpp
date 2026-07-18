#include "SetTrackColor.h"

#include "../../../controllers/SongController.h"
#include "../../../ui/utils/UiColorUtil.h"

namespace arkostracker
{

SetTrackColor::SetTrackColor(SongController& pSongController, Id pSubsongId, const int pPositionIndex, const int pChannelIndex,
    const OptionalValue<juce::Colour>& pNewColor) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex),
        newColor(pNewColor),
        oldColor()
{
}

bool SetTrackColor::perform()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo.
        const auto color = subsong.getConstTrackRefFromPosition(positionIndex, channelIndex).getColor();
        oldColor = color.isPresent() ? OptionalValue(juce::Colour(color.getValue())) : OptionalValue<juce::Colour>();
    });

    // Any change? If not, don't do anything.
    if (newColor == oldColor) {
        return false;
    }

    // Sets the new data.
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setTrackColor(positionIndex, channelIndex, UiColorUtil::toUInt32(newColor));
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool SetTrackColor::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setTrackColor(positionIndex, channelIndex, UiColorUtil::toUInt32(oldColor));
    });

    // Notifies.
    notifyListeners();

    return true;
}

void SetTrackColor::notifyListeners() const noexcept
{
    songController.getTrackObservers().applyOnObservers([&] (TrackChangeObserver* observer) noexcept {
        observer->onTrackMetaDataChanged(subsongId);
    });
}

}   // namespace arkostracker
