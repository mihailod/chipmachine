#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "InstrumentAction.h"

namespace arkostracker 
{

/** Action to set the color of an instrument. */
class SetInstrumentColor final : public InstrumentAction
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param id the ID of the instrument to modify.
     * @param newColor the new color.
     */
    SetInstrumentColor(SongController& songController, Id id, juce::Colour newColor) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies of the change. */
    void notify() noexcept;

    const Id id;
    juce::Colour newColor;
    juce::Colour oldColor;
};

}   // namespace arkostracker
