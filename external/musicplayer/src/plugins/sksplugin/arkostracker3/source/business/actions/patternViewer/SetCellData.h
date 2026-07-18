#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../../../song/Location.h"
#include "../../../song/cells/Cell.h"
#include "../../../utils/OptionalValue.h"
#include "../../../ui/patternViewer/controller/dataItem/EffectDigit.h"
#include "../../../ui/patternViewer/controller/dataItem/EffectNumber.h"
#include "../../../ui/patternViewer/controller/dataItem/Digit.h"

namespace arkostracker 
{

class SongController;

/** Action to set the data on a Cell, mixing its content with the existing data. Useful when writing data from the PatternViewer. */
class SetCellData final : public juce::UndoableAction
{
public:
    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param location the location of the position.
     * @param channelIndex the channel index.
     * @param newNote the possible new note to write.
     * @param newInstrument the possible new instrument to write.
     * @param newEffectDigit an effect digit, or absent if none must be written.
     * @param newEffectNumber an effect number, or absent if none must be written.
     * @param newInstrumentDigit an instrument digit, or absent if none must be written.
     */
    SetCellData(SongController& songController, Location location, int channelIndex, OptionalValue<Note> newNote, OptionalValue<int> newInstrument,
                OptionalValue<EffectDigit> newEffectDigit, OptionalValue<EffectNumber> newEffectNumber, OptionalValue<Digit> newInstrumentDigit) noexcept;

    bool perform() override;
    bool undo() override;

private:
    /** Notifies the Listeners of a change. */
    void notifyListeners() noexcept;

    SongController& songController;

    const Location location;
    const int channelIndex;
    const OptionalValue<Note> newNote;
    const OptionalValue<int> newInstrument;
    const OptionalValue<EffectDigit> newEffectDigit;
    const OptionalValue<EffectNumber> newEffectNumber;
    const OptionalValue<Digit> newInstrumentDigit;

    Cell oldCell;
};



}   // namespace arkostracker

