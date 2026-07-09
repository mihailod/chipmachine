#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"
#include "../../../utils/OptionalValue.h"

namespace arkostracker
{

class SongController;

/** Action to modify Positions: only the height, but also the Patterns (the color). */
class ModifyPositionsData final : public juce::UndoableAction
{
public:
    /**
    * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positionIndexes the indexes of the position. Must be valid.
     * @param newHeight the possible new height.
     * @param newPatternColorArgb the possible new color.
     */
    ModifyPositionsData(SongController& songController, Id subsongId, const std::set<int>& positionIndexes, OptionalInt newHeight,
                        OptionalValue<juce::uint32> newPatternColorArgb) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const std::set<int> positionIndexes;
    OptionalInt newHeight;
    OptionalValue<juce::uint32> newPatternColorArgb;

    std::map<int, Position> oldPositionIndexToPosition;
    std::map<int, Pattern> oldPatternIndexToPattern;
};

}   // namespace arkostracker
