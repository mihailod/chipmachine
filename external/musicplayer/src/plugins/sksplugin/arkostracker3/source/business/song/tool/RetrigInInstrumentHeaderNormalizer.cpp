#include "RetrigInInstrumentHeaderNormalizer.h"

#include "../../../song/Song.h"

namespace arkostracker 
{

RetrigInInstrumentHeaderNormalizer::RetrigInInstrumentHeaderNormalizer(Song& pSong) noexcept :
        song(pSong)
{
}

void RetrigInInstrumentHeaderNormalizer::apply() noexcept
{
    song.performOnInstruments([&](std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (auto& instrument : instruments) {
            transformInstrumentIfNeeded(*instrument);
        }
    });
}

void RetrigInInstrumentHeaderNormalizer::transformInstrumentIfNeeded(Instrument& instrument) noexcept
{
    // Psg Instrument?
    if (instrument.getType() != InstrumentType::psgInstrument) {
        return;
    }

    auto& psgPart = instrument.getPsgPart();

    // Nothing to do if the Retrig is off!
    if (!psgPart.isInstrumentRetrig()) {
        return;
    }

    jassert(psgPart.getLength() > 0);
    const auto loop = psgPart.getMainLoop();
    const auto isLooping = loop.isLooping();
    auto firstCell = psgPart.getCellRefConst(0);

    // No more Instrument Retrig, it will be transferred!
    psgPart.setInstrumentRetrig(false);

    // Optimization: the Retrig must be useful: the first line must be a hardware sound!
    // If not, then the retrig has been removed, we can stop here.
    if (!PsgInstrumentCellLinkHelper::isHardware(firstCell.getLink())) {
        return;
    }

    // Also, nothing more to do if there is a Retrig on the first line AND that there is a loop AFTER it (because the user has already managed the behavior by himself).
    const auto loopStart = loop.getStartIndex();
    if (isLooping && firstCell.isRetrig() && (loopStart > 0)) {
        return;
    }

    // If no loop and there is a Retrig on the first line (the added line will serve no purpose), we can exit.
    if (!isLooping && firstCell.isRetrig()) {
        return;
    }

    // Encodes the first line.
    // By doing so, the possible looping is automatically managed!
    // Creates a new Cell with a set Retrig.
    const auto firstCellWithRetrig = firstCell.withRetrig(true);
    // Replaces the first line with the same, but with a Retrig.
    psgPart.setCell(0, firstCellWithRetrig);

    // If not looping and there is only one line, nothing else to do.
    if (!isLooping && (loop.getEndIndex() == 0)) {
        return;
    }

    // If not looping, if the loop was on the next lines, there is nothing more to do.
    if (!isLooping && loopStart > 0) {
        return;
    }

    // Loops on 0. Creates a new entry.
    const int newEndIndex = loop.getEndIndex() + 1;
    const int newStartIndex = loop.getStartIndex() + 1;

    // A last line, equal to the first one (hence, without Retrig, we tested it before), is added at the end.
    psgPart.insertCell(newEndIndex, firstCell);

    // Both then start/end indexes are increased.
    psgPart.setMainLoop(Loop(newStartIndex, newEndIndex, isLooping));
}

}   // namespace arkostracker

