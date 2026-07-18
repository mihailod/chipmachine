#include "SetPsgInstrumentCell.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker
{

SetPsgInstrumentCell::SetPsgInstrumentCell(SongController& pSongController, Id pId, int pCellIndex, const PsgInstrumentCell& pCell) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        createdBarCount(0),
        refreshIndexStart(cellIndex),       // By default.
        originalEnd(0),
        newEnd(0),
        originalAutoSpreadLoops(),
        mustRefreshAllAfterToo(false),
        newCell(pCell),
        oldCell()
{
}

bool SetPsgInstrumentCell::perform()
{
    if (cellIndex < 0) {
        jassertfalse;           // Shouldn't happen!
        return false;
    }

    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();
        const auto initialLength = psgPart.getLength();
        const auto mainLoop = psgPart.getMainLoop();
        originalEnd = mainLoop.getEndIndex();
        newEnd = originalEnd;           // For now.
        originalAutoSpreadLoops = psgPart.getAutoSpreadAllLoops();
        auto modifiedAutoSpreadLoops = psgPart.getAutoSpreadAllLoops();

        // The instrument may be sized-increased if the cell index is >.
        if (cellIndex >= initialLength) {
            // Must create new Cells. There is NO old cells. OldCell shouldn't be used.
            createdBarCount = cellIndex - initialLength + 1;
            for (auto i = 0; i < createdBarCount; ++i) {
                psgPart.addCell(newCell);
            }
            mustRefreshAllAfterToo = true;
            refreshIndexStart = initialLength;      // Refreshes from here.

            // If the loop was at the end, changes it. Handy for the user.
            const auto lastIndex = (initialLength - 1);
            if (originalEnd == lastIndex) {
                newEnd += createdBarCount;
            }

            // Does the auto-spread need to be modified?
            for (auto& entry : modifiedAutoSpreadLoops) {
                const auto& psgSection = entry.first;
                const auto loop = entry.second;
                if (loop.getEndIndex() == lastIndex) {
                    const auto newLoop = loop.withEnd(newEnd);
                    modifiedAutoSpreadLoops.at(psgSection) = newLoop;
                }
                psgPart.setAutoSpreadLoops(modifiedAutoSpreadLoops);
            }
        } else {
            // Classic behavior: Sets an cell over an existing one.
            // Saves the old Cell, for Undo.
            createdBarCount = 0;
            oldCell = psgPart.getCellRefConst(cellIndex);

            // Sets the new one.
            psgPart.setCell(cellIndex, newCell);

            // Sets the end? Handy if the loop end is before the modified Cell.
            if (cellIndex > originalEnd) {
                newEnd = cellIndex;
            }

            // Has "something" with auto-spread on has been modified? If yes, a flag is set to indicate the observers
            // the next items have been impacted.
            mustRefreshAllAfterToo = (oldCell.getLink() != newCell.getLink()) && psgPart.isAutoSpread(PsgSection::soundType);
            // For Envelope, considers the volume, the ratio and the hard env.
            const auto isEnvelopeAutoSpread = psgPart.isAutoSpread(PsgSection::envelope);
            mustRefreshAllAfterToo |= isEnvelopeAutoSpread && (oldCell.getVolume() != newCell.getVolume());
            mustRefreshAllAfterToo |= isEnvelopeAutoSpread && (oldCell.getHardwareEnvelope() != newCell.getHardwareEnvelope());
            mustRefreshAllAfterToo |= isEnvelopeAutoSpread && (oldCell.getRatio() != newCell.getRatio());

            mustRefreshAllAfterToo |= (oldCell.getNoise() != newCell.getNoise()) && psgPart.isAutoSpread(PsgSection::noise);

            mustRefreshAllAfterToo |= (oldCell.getPrimaryPeriod() != newCell.getPrimaryArpeggioNoteInOctave()) && psgPart.isAutoSpread(PsgSection::primaryPeriod);
            mustRefreshAllAfterToo |= (oldCell.getPrimaryArpeggioNoteInOctave() != newCell.getPrimaryArpeggioNoteInOctave())
                                      && psgPart.isAutoSpread(PsgSection::primaryArpeggioNoteInOctave);
            mustRefreshAllAfterToo |= (oldCell.getPrimaryArpeggioOctave() != newCell.getPrimaryArpeggioOctave()) && psgPart.isAutoSpread(PsgSection::primaryArpeggioOctave);
            mustRefreshAllAfterToo |= (oldCell.getPrimaryPitch() != newCell.getPrimaryPitch()) && psgPart.isAutoSpread(PsgSection::primaryPitch);

            mustRefreshAllAfterToo |= (oldCell.getSecondaryPeriod() != newCell.getSecondaryArpeggioNoteInOctave()) && psgPart.isAutoSpread(PsgSection::secondaryPeriod);
            mustRefreshAllAfterToo |= (oldCell.getSecondaryArpeggioNoteInOctave() != newCell.getSecondaryArpeggioNoteInOctave())
                                      && psgPart.isAutoSpread(PsgSection::secondaryArpeggioNoteInOctave);
            mustRefreshAllAfterToo |= (oldCell.getSecondaryArpeggioOctave() != newCell.getSecondaryArpeggioOctave()) && psgPart.isAutoSpread(PsgSection::secondaryArpeggioOctave);
            mustRefreshAllAfterToo |= (oldCell.getSecondaryPitch() != newCell.getSecondaryPitch()) && psgPart.isAutoSpread(PsgSection::secondaryPitch);

            mustRefreshAllAfterToo |= (oldCell.isSidActivated() != newCell.isSidActivated()) && psgPart.isAutoSpread(PsgSection::sidIsActivated);
            mustRefreshAllAfterToo |= (oldCell.getSidRatio() != newCell.getSidRatio()) && psgPart.isAutoSpread(PsgSection::sidIsActivated); // Encoded with IsActivated.
            mustRefreshAllAfterToo |= (oldCell.getSidBalancePercent() != newCell.getSidBalancePercent()) && psgPart.isAutoSpread(PsgSection::sidBalancePercent);
            mustRefreshAllAfterToo |= (oldCell.getSidLowShelfVolume() != newCell.getSidLowShelfVolume()) && psgPart.isAutoSpread(PsgSection::sidLowShelfVolume);
            mustRefreshAllAfterToo |= (oldCell.getSidArpeggio() != newCell.getSidArpeggio()) && psgPart.isAutoSpread(PsgSection::sidArpeggio);
            mustRefreshAllAfterToo |= (oldCell.getSidPitch() != newCell.getSidPitch()) && psgPart.isAutoSpread(PsgSection::sidPitch);
        }

        // Changes the end.
        const auto newLoop = mainLoop.withEnd(newEnd);
        psgPart.setMainLoop(newLoop);
    });

    notify();

    return true;
}

bool SetPsgInstrumentCell::undo()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();

        psgPart.setMainLoop(psgPart.getMainLoop().withEnd(originalEnd));

        // Should the instrument be reduced? If yes, nothing else to do, the new data is in the removed part.
        if (createdBarCount > 0) {
            psgPart.removeCells(refreshIndexStart, createdBarCount);
            psgPart.setAutoSpreadLoops(originalAutoSpreadLoops);
        } else {
            // Sets the old Cell.
            psgPart.setCell(cellIndex, oldCell);
        }
    });

    notify();

    return true;
}

void SetPsgInstrumentCell::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        // If adding new cells, all of them must be refreshed.
        observer->onPsgInstrumentCellChanged(id, refreshIndexStart, mustRefreshAllAfterToo);
        if (newEnd != originalEnd) {
            // Notifies the end change.
            observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
        }
    });
}

}   // namespace arkostracker
