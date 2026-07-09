#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/Id.h"

namespace arkostracker 
{

class SongController;

/** Action to modify a Marker from a Position. */
class ModifyPositionMarker final : public juce::UndoableAction              // TO IMPROVE: This not used anymore (for now).
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the index of the Position. Must be valid.
     * @param newName the new name. May be empty to "delete" the marker (only hidden).
     * @param newColor the new color. Irrelevant if hidden.
     */
    ModifyPositionMarker(SongController& songController, Id subsongId, int positionIndex, juce::String newName, juce::Colour newColor) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionIndex;
    const juce::String newName;
    const juce::Colour newColor;

    juce::String oldName;
    juce::Colour oldColor;
};

}   // namespace arkostracker
