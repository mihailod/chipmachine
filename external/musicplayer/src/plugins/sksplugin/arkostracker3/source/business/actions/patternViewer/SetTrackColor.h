#pragma once

#include "../../../utils/Id.h"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/OptionalValue.h"

namespace arkostracker
{

class SongController;

/** Action to set the color of a Track. */
class SetTrackColor final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param positionIndex the index of the Position to modify. Must be valid.
     * @param channelIndex the channel index. Must be valid.
     * @param newColor the new color. May be empty not to use a color.
     */
    SetTrackColor(SongController& songController, Id subsongId, int positionIndex, int channelIndex, const OptionalValue<juce::Colour>& newColor) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() const noexcept;

    SongController& songController;

    const Id subsongId;
    const int positionIndex;
    const int channelIndex;
    OptionalValue<juce::Colour> newColor;

    OptionalValue<juce::Colour> oldColor;
};

}   // namespace arkostracker
