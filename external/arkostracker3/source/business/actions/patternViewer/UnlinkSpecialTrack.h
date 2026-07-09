#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../utils/Id.h"

namespace arkostracker
{

class SongController;

/** Action to unlink a Special Track. */
class UnlinkSpecialTrack final : public juce::UndoableAction {

public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param positionIndex the index of the position from which to unlink.
     */
    UnlinkSpecialTrack(SongController& songController, Id subsongId, bool isSpeedTrack, int positionIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Id subsongId;
    const bool isSpeedTrack;
    const int positionIndex;

    Pattern originalPattern;
    int sourcePatternIndex;
};

}   // namespace arkostracker
