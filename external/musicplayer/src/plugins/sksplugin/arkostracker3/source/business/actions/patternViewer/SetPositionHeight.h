#pragma once

#include <memory>

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/Location.h"
#include "../../../song/subsong/Position.h"

namespace arkostracker 
{

class SongController;

/** Action to modify a Position (replace a Position with another one). */
class SetPositionHeight final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param location the location of the position.
     * @param newHeight the new height.
     */
    SetPositionHeight(SongController& songController, Location location, int newHeight) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Location location;
    const int newHeight;

    std::unique_ptr<Position> oldPosition;
};

}   // namespace arkostracker

