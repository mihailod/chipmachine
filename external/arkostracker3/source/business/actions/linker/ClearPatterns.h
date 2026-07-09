#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../utils/Id.h"

namespace arkostracker
{

class Subsong;
class SongController;

/** Action to clear the Patterns and Tracks, and create one position. */
class ClearPatterns final : public juce::UndoableAction
{
public:

    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     */
    ClearPatterns(SongController& songController, Id subsongId) noexcept;

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

    std::unique_ptr<Subsong> storedSubsong;
};

}   // namespace arkostracker
