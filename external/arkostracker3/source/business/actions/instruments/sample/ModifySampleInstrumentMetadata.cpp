#include "ModifySampleInstrumentMetadata.h"

#include "../../../../controllers/SongController.h"
#include "../../../../song/cells/CellConstants.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "../../../song/validation/CheckLoopStartEnd.h"

namespace arkostracker
{

ModifySampleInstrumentMetadata::ModifySampleInstrumentMetadata(SongController& pSongController, Id pId, const OptionalInt pNewLoopStart, const OptionalInt pNewEnd,
                                                               const OptionalBool pNewIsLoop, const OptionalFloat pNewAmplification, const OptionalInt pNewDigiNote) noexcept:
        songController(pSongController),
        id(std::move(pId)),
        loopStart(pNewLoopStart),
        end(pNewEnd),
        isLooping(pNewIsLoop),
        amplification(pNewAmplification),
        digiNote(pNewDigiNote),
        oldLoop(),
        oldAmplification(0.0F),
        oldDigiNote(PsgValues::digidrumNote)
{
    jassert(loopStart.isPresent() || end.isPresent() || isLooping.isPresent() || amplification.isPresent() || digiNote.isPresent());    // No new data??
}

bool ModifySampleInstrumentMetadata::perform()
{
    auto change = false;

    songController.performOnInstrument(id, [&] (Instrument& instrument) noexcept {
        jassert(instrument.getType() == InstrumentType::sampleInstrument);

        auto& samplePart = instrument.getSamplePart();

        // Saves the old data, for Undo.
        oldLoop = samplePart.getLoop();
        oldAmplification = samplePart.getAmplificationRatio();
        oldDigiNote = samplePart.getDigidrumNote();
        const auto length = samplePart.getSample()->getLength();

        // Sets the new data, if present. Corrects it.
        const auto [correctedStart, correctedEnd] = CheckLoopStartEnd::checkLoopStartEnd(oldLoop.getStartIndex(), oldLoop.getEndIndex(), length,
                                                                                         loopStart, end);
        Loop newLoop(correctedStart, correctedEnd, oldLoop.isLooping());
        if (isLooping.isPresent()) {
            newLoop = newLoop.withLooping(isLooping.getValue());
        }

        if (newLoop != oldLoop) {
            samplePart.setLoop(newLoop);
            change = true;
        }

        if (digiNote.isPresent()) {
            const auto newDigiNote = NumberUtil::correctNumber(digiNote.getValue(), CellConstants::minimumNote, CellConstants::maximumNote);
            samplePart.setDigidrumNote(newDigiNote);
            change = true;
        }

        if (amplification.isPresent()) {
            const auto newAmplification = amplification.getValue();
            const auto correctedAmplification = NumberUtil::correctNumber(newAmplification,
                                                                          static_cast<float>(PsgValues::sampleMultiplierMinimumValue),
                                                                          static_cast<float>(PsgValues::sampleMultiplierMaximumValue));
            if (!juce::exactlyEqual(correctedAmplification, oldAmplification)) {
                samplePart.setAmplificationRatio(correctedAmplification);
                change = true;
            }
        }
    });

    if (change) {
        notify();
    }

    return change;
}

bool ModifySampleInstrumentMetadata::undo()
{
    // Writes back all the stored data.
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& samplePart = instrument.getSamplePart();

        samplePart.setAmplificationRatio(oldAmplification);
        samplePart.setLoop(oldLoop);
        samplePart.setDigidrumNote(oldDigiNote);
    });

    notify();

    return true;
}

void ModifySampleInstrumentMetadata::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker
