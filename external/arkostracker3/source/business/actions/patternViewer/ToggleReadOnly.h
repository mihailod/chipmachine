#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <set>

#include "../../../utils/OptionalValue.h"

namespace arkostracker
{

class SongController;

/** Action to toggle the read-only state of one or several Tracks. */
class ToggleReadOnly final : public juce::UndoableAction
{
public:
    /**
     * Transposes a part of the Subsong.
     * @param songController the Song Controller.
     * @param subsongId the ID of the Subsong, useful for Notifications.
     * @param trackIndexes the track indexes. May be empty.
     * @param speedTrackIndex index if the speed track must be toggled.
     * @param eventTrackIndex index if the event track must be toggled.
     */
    ToggleReadOnly(SongController& songController, Id subsongId, const std::set<int>& trackIndexes, OptionalInt speedTrackIndex, OptionalInt eventTrackIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;
    const Id subsongId;

    const std::set<int> originalTrackIndexes;
    const OptionalInt originalSpeedTrackIndex;
    const OptionalInt originalEventTrackIndex;
};

}   // namespace arkostracker
