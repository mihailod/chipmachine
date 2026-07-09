#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/subsong/Pattern.h"
#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"

namespace arkostracker
{

class Subsong;
class SongController;

/** Action to rearrange the patterns, to get them linearly. */
class RearrangePatterns final : public juce::UndoableAction
{
public:

    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param deleteUnusedPatterns true to delete unused patterns.
     */
    RearrangePatterns(SongController& songController, Id subsongId, bool deleteUnusedPatterns) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /**
     * Notifies the Listeners of a change.
     * @param subsongIdToShow the ID of the Subsong to show.
     */
    void notify(const Id& subsongIdToShow) const noexcept;

    SongController& songController;

    const Id subsongId;
    Id newSubsongId;
    bool deleteUnusedPatterns;

    std::vector<Position> oldPositions;
    std::vector<Pattern> oldPatterns;
};

}   // namespace arkostracker
