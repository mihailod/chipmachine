#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../utils/Id.h"

namespace arkostracker
{

class SongController;

/** Action to link a Special Track. */
class LinkSpecialTrack final : public juce::UndoableAction
{

public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     */
    LinkSpecialTrack(SongController& songController, Id subsongId, bool isSpeedTrack, int sourcePositionIndex, int targetPositionIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Id subsongId;
    const bool isSpeedTrack;
    const int sourcePositionIndex;
    const int targetPositionIndex;

    Pattern originalPattern;
    int sourcePatternIndex;
};

}   // namespace arkostracker
