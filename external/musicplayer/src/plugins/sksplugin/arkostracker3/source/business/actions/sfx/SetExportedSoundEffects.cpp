#include "SetExportedSoundEffects.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

SetExportedSoundEffects::SetExportedSoundEffects(SongController& pSongController, const std::unordered_set<int>& pExportedInstrumentIndexes) noexcept :
    songController(pSongController),
    newExportedInstrumentIndexes(pExportedInstrumentIndexes),
    oldExportedInstrumentIndexes()
{
}

bool SetExportedSoundEffects::perform()
{
    auto change = false;

    oldExportedInstrumentIndexes.clear();

    auto instrumentIndex = 0;
    songController.performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (auto& instrument : instruments) {
            if ((instrumentIndex > 0) && (instrument->getType() == InstrumentType::psgInstrument)) {
                auto& psgPart = instrument->getPsgPart();
                // Any change?
                const auto newExportedState = (newExportedInstrumentIndexes.find(instrumentIndex) != newExportedInstrumentIndexes.cend());
                const auto oldExportedState = psgPart.isSoundEffectExported();
                if (oldExportedState != newExportedState) {
                    psgPart.setSoundEffectExported(newExportedState);

                    change = true;
                }

                // Stores the exported flag for the undo.
                if (oldExportedState) {
                    oldExportedInstrumentIndexes.insert(instrumentIndex);
                }
            }

            ++instrumentIndex;
        }
    });

    return change;
}

bool SetExportedSoundEffects::undo()
{
    auto instrumentIndex = 0;
    songController.performOnInstruments([&] (std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (auto& instrument : instruments) {
            // Skips the instrument 0 and samples.
            if ((instrumentIndex != 0) && (instrument->getType() == InstrumentType::psgInstrument)) {
                auto& psgPart = instrument->getPsgPart();
                const auto exportedState = (oldExportedInstrumentIndexes.find(instrumentIndex) != oldExportedInstrumentIndexes.cend());
                psgPart.setSoundEffectExported(exportedState);
            }

            ++instrumentIndex;
        }
    });

    return true;
}

}   // namespace arkostracker
