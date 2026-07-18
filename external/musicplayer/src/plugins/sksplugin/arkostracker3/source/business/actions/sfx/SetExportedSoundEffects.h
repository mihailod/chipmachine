#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace arkostracker
{

class SongController;

/**
 * Sets the sound effects to exported or not. Nothing happens if there is no change. Instrument 0 is ignored.
 * Note that we don't notify anyone of the result, as it doesn't seem useful.
 */
class SetExportedSoundEffects : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param exportedInstrumentIndexes the exported instrument indexes.
     */
    SetExportedSoundEffects(SongController& songController, const std::unordered_set<int>& exportedInstrumentIndexes) noexcept;

    bool perform() override;
    bool undo() override;

private:
    SongController& songController;
    std::unordered_set<int> newExportedInstrumentIndexes;
    std::unordered_set<int> oldExportedInstrumentIndexes;
};

}   // namespace arkostracker
