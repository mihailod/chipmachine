#pragma once

#include "../../../song/instrument/InstrumentType.h"

namespace arkostracker 
{

/** Provides specialized data for the Instrument items. */
class InstrumentDataController
{
public:
    /** Destructor. */
    virtual ~InstrumentDataController() = default;

    /** @return the color of the current instrument, which index is given. */
    virtual juce::Colour getInstrumentColor(int instrumentIndex) const noexcept = 0;

    /**
     * Sets the color of an instrument.
     * @param instrumentIndex the index of the Instrument.
     * @param color the color.
     */
    virtual void setInstrumentColor(int instrumentIndex, juce::Colour color) noexcept = 0;

    /** @return the type of the Instrument, which index is given. */
    virtual InstrumentType getInstrumentType(int instrumentIndex) const noexcept = 0;

    /**
     * Called when the instrument symbol is clicked.
     * @param instrumentIndex the instrument index.
     */
    virtual void onInstrumentSymbolClicked(int instrumentIndex) noexcept = 0;
};

}   // namespace arkostracker
