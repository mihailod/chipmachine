#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../utils/Id.h"

namespace arkostracker
{

class SongController;

class LinkTrack final : public juce::UndoableAction {

public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param sourcePositionIndex the index of the position from which to link.
     * @param sourceChannelIndex the channel index from which to link.
     * @param targetPositionIndex  the index of the position to link to.
     * @param targetChannelIndex the channel index to link to.
     */
    LinkTrack(SongController& songController, Id subsongId, int sourcePositionIndex, int sourceChannelIndex, int targetPositionIndex, int targetChannelIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int sourcePositionIndex;
    const int sourceChannelIndex;
    const int targetPositionIndex;
    const int targetChannelIndex;

    Pattern originalPattern;
    int sourcePatternIndex;
};

}   // namespace arkostracker
