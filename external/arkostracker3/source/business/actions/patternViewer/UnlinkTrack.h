#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../utils/Id.h"

namespace arkostracker
{

class SongController;

class UnlinkTrack final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the index of the position from which to unlink.
     * @param channelIndex the channel index from which to unlink.
     */
    UnlinkTrack(SongController& songController, Id subsongId, int positionIndex, int channelIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionIndex;
    const int channelIndex;

    Pattern originalPattern;
    int sourcePatternIndex;
};

}   // namespace arkostracker
