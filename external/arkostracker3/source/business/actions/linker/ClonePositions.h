#pragma once

#include <set>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"

namespace arkostracker 
{

class Pattern;
class Subsong;
class SongController;

/**
 * Action to clone Positions. This creates new Patterns. Note that the new Track names are forced to blank,
 * to make the Link feature work better. The markers are also cleared.
 */
class ClonePositions final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positions the positions to clone.
     * @param destinationPosition where to insert the positions.
     * @param insertAfter true if the Positions must be put after the destination, false if before.
     */
    ClonePositions(SongController& songController, Id subsongId, std::set<int> positions, int destinationPosition, bool insertAfter) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the Listeners of a change.
     * @param highlightedItems the items to select after the action.
     */
    void notifyListeners(const std::set<int>& highlightedItems) const noexcept;

    void cloneSpecialTrack(bool isSpeedTrack, Subsong& subsong, int positionIndex, const Pattern& originalPattern, Pattern& newPattern) noexcept;

    SongController& songController;

    const Id subsongId;
    const std::set<int> sourcePositionIndexes;
    const int destinationPosition;
    const bool insertAfter;

    // The following are useful for the Undo, to know how many items must be deleted.
    int addedTrackCount;
    int addedSpeedTrackCount;
    int addedEventTrackCount;
    int addedPatternCount;
};

}   // namespace arkostracker
