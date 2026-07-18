#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>
#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to move a Marker. The source Marker is emptied. */
class MovePositionMarker final : public juce::UndoableAction              // TO IMPROVE: This not used anymore (for now).
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param sourcePosition the index of the source Position. Must be valid.
     * @param destinationPosition the index of the destination Position. Must be valid.
     */
    MovePositionMarker(SongController& songController, Id subsongId, int sourcePosition, int destinationPosition) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int sourcePosition;
    const int destinationPosition;
    //juce::String sourceNewName;
    //juce::Colour sourceNewColor;
    juce::String sourceOldName;
    juce::Colour sourceOldColor;

    //juce::String destinationNewName;
    //juce::Colour destinationNewColor;
    juce::String destinationOldName;
    juce::Colour destinationOldColor;
};

}   // namespace arkostracker

