#pragma once

#include "../../../utils/Id.h"

#include <juce_data_structures/juce_data_structures.h>

namespace arkostracker
{

class SongController;

/** Action to set the name of a Special Track. */
class SetSpecialTrackName final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positionIndex the index of the Position to modify. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event.
     * @param name the new name.
     */
    SetSpecialTrackName(SongController& songController, Id subsongId, int positionIndex, bool isSpeedTrack, const juce::String& name) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionIndex;
    const bool isSpeedTrack;
    const juce::String newName;

    juce::String oldName;
};

}   // namespace arkostracker
