#pragma once

#include "InstrumentAction.h"

namespace arkostracker 
{

/** Action to rename an instrument. */
class RenameInstrument final : public InstrumentAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the ID of the instrument to rename.
     * @param newName the new name.
     */
    RenameInstrument(SongController& songController, Id id, juce::String newName) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies of the change. */
    void notify() noexcept;

    const Id id;
    juce::String newName;
    juce::String oldName;
};



}   // namespace arkostracker

