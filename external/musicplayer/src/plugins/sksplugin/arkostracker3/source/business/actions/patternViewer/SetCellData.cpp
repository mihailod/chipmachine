#include "SetCellData.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

SetCellData::SetCellData(SongController& pSongController, Location pLocation, int pChannelIndex, OptionalValue<Note> pNewNote, OptionalValue<int> pNewInstrument,
                         OptionalValue<EffectDigit> pNewEffectDigit, OptionalValue<EffectNumber> pNewEffectNumber, OptionalValue<Digit> pNewInstrumentDigit) noexcept :
        songController(pSongController),
        location(std::move(pLocation)),
        channelIndex(pChannelIndex),
        newNote(pNewNote),
        newInstrument(pNewInstrument),
        newEffectDigit(pNewEffectDigit),
        newEffectNumber(pNewEffectNumber),
        newInstrumentDigit(pNewInstrumentDigit),
        oldCell()
{
}

bool SetCellData::perform()
{
    // Makes sure there is something to do.
    if (newNote.isAbsent() && newInstrument.isAbsent() && newEffectDigit.isAbsent() && newEffectNumber.isAbsent() && newInstrumentDigit.isAbsent()) {
        jassertfalse;       // Nothing to change? Strange.
        return false;
    }
    jassert(newInstrument.isAbsent() || newInstrumentDigit.isAbsent());     // Abnormal that both are present!

    const auto position = location.getPosition();
    const auto line = location.getLine();

    songController.performOnSubsong(location.getSubsongId(), [&](Subsong& subsong) noexcept {
        // Saves the Cell for undo, but also gets it to modify it.
        auto readCell = subsong.getCell(position, line, channelIndex);
        oldCell = readCell;

        // Modifies the cell.
        if (newNote.isPresent()) {
            readCell.setNote(newNote);
        }
        if (newInstrument.isPresent()) {
            readCell.setInstrument(newInstrument);
        }
        if (newEffectDigit.isPresent()) {
            const auto& effectDigit = newEffectDigit.getValueRef();
            readCell.setEffectDigit(effectDigit.effectIndex, effectDigit.digitIndex, effectDigit.value);
        }
        if (newEffectNumber.isPresent()) {
            const auto& effectNumber = newEffectNumber.getValueRef();
            readCell.setEffectNumber(effectNumber.effectIndex, effectNumber.effect);
        }
        if (newInstrumentDigit.isPresent()) {
            const auto instrumentDigitData = newInstrumentDigit.getValue();
            const auto instrumentIndexOptional = readCell.getInstrument();
            const auto baseInstrumentIndex = instrumentIndexOptional.isPresent() ? instrumentIndexOptional.getValue() : 0;
            const auto newInstrumentIndex = NumberUtil::replaceDigit(baseInstrumentIndex, instrumentDigitData.digitIndex, instrumentDigitData.digitValue);
            readCell.setInstrument(newInstrumentIndex);
        }

        // Stores it.
        subsong.setCell(position, line, channelIndex, readCell);
    });

    // Notifies the listeners of the change.
    notifyListeners();

    return true;
}

bool SetCellData::undo()
{
    const auto position = location.getPosition();
    const auto line = location.getLine();

    songController.performOnSubsong(location.getSubsongId(), [&](Subsong& subsong) noexcept {
        // Writes the previously saved cell.
        subsong.setCell(position, line, channelIndex, oldCell);
    });

    // Notifies the listeners of the change.
    notifyListeners();

    return true;
}

void SetCellData::notifyListeners() noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(location.getSubsongId());
    });
}

}   // namespace arkostracker
