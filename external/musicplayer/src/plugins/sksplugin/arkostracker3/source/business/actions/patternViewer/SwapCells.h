#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"
#include "../../song/tool/browser/CellAndLocation.h"

namespace arkostracker
{

class SongController;

/** Swaps the Cells of two tracks. Tracks are written only once to avoid strange behavior with linked tracks. */
class SwapCells final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionStartIndex the index of the start position to work on.
     * @param positionEndIndex the index of the end position to work on.
     * @param topLineIndex the top line to start from. 0 if all the track is taken in account.
     * @param bottomLineIndex the bottom line to end with. 127 if all the track is taken in account.
     * @param firstChannelIndex one of the channel to swap. If not valid, the swap is not performed.
     * @param secondChannelIndex the other the channel to swap. If not valid, the swap is not performed.
     */
    SwapCells(SongController& songController, Id subsongId, int positionStartIndex, int positionEndIndex, int topLineIndex, int bottomLineIndex,
        int firstChannelIndex, int secondChannelIndex) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionStartIndex;
    const int positionEndIndex;
    const int topLineIndex;
    const int bottomLineIndex;
    const int firstChannelIndex;
    const int secondChannelIndex;

    std::vector<CellAndLocation> undoData;
};

}   // namespace arkostracker
