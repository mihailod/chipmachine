#include "InstrumentOptimizer.h"

#include "../../../../song/instrument/Instrument.h"
#include "../RetrigInInstrumentHeaderNormalizer.h"

namespace arkostracker 
{

InstrumentOptimizer::InstrumentOptimizer(Instrument& pInstrument, const bool pTransferInstrumentRetrig, const bool pFlatten) noexcept :
        BaseItemOptimizer(pInstrument),
        transferInstrumentRetrig(pTransferInstrumentRetrig),
        flatten(pFlatten)
{
}

void InstrumentOptimizer::optimize() noexcept
{
    auto& item = getItem();

    // Only PSGs Instrument can be optimized.
    if (item.getType() != InstrumentType::psgInstrument) {
        return;
    }

    // First, flattens all the Cells of the Instrument.
    if (flatten) {
        flattenInstrument();
    }

    if (flatten && transferInstrumentRetrig) {
        RetrigInInstrumentHeaderNormalizer::transformInstrumentIfNeeded(item);
    }

    removePastEndCells();
    convertMuteCells();
    removeLastMuteCells();
    removeMuteLoop();
    // This would be too unlikely to make it work without flattening.
    if (flatten) {
        removeDuplicateCells();
        removeCycles();
        optimizePreLoop();
    }
}

void InstrumentOptimizer::flattenInstrument() noexcept
{
    auto& item = getItem();
    auto& psgPart = item.getPsgPart();

    psgPart.performOnEachCell([&](const int cellIndex, const PsgInstrumentCell& /*originalCell*/) noexcept {
        const auto spreadCell = psgPart.buildSpreadPsgInstrumentCell(cellIndex);
        const auto& newCell = spreadCell.getCellWithAutoSpreadAppliedRef();
        psgPart.setCell(cellIndex, newCell);
    });
    // No more auto-loops!
    psgPart.setAutoSpreadLoops({ });
}

void InstrumentOptimizer::convertMuteCells() noexcept
{
    auto& item = getItem();
    auto& psgPart = item.getPsgPart();

    psgPart.performOnEachCell([&](const int cellIndex, const PsgInstrumentCell& cell) noexcept{
        if ((cell.getLink() == PsgInstrumentCellLink::softOnly) && (cell.getVolume() == 0)) {
            const auto newCell = PsgInstrumentCell::getEmptyPsgInstrumentCell();
            psgPart.setCell(cellIndex, newCell);
        }
    });
}

void InstrumentOptimizer::removeLastMuteCells() noexcept
{
    auto& item = getItem();
    auto& psgPart = item.getPsgPart();
    const auto& loop = psgPart.getMainLoopRef();
    if (loop.isLooping()) {
        return;
    }

    const auto endIndex = loop.getEndIndex();
    const auto length = psgPart.getLength();
    jassert(endIndex == length - 1);       // No past-end cells must be present.

    auto cellCountToRemove = 0;
    auto cellIndex = length - 1;
    auto mustContinue = true;
    while (mustContinue && (cellIndex > 0)) {
        if (psgPart.getCellRefConst(cellIndex).isMute()) {
            ++cellCountToRemove;
        } else {
            mustContinue = false;
        }
        --cellIndex;
    }

    if (cellCountToRemove > 0) {
        psgPart.removeCells(length - cellCountToRemove, cellCountToRemove);
        psgPart.setMainLoop(Loop(0, endIndex - cellCountToRemove));
    }
}

void InstrumentOptimizer::removeMuteLoop() noexcept
{
    auto& item = getItem();
    auto& psgPart = item.getPsgPart();
    const auto length = psgPart.getLength();
    const auto& loop = psgPart.getMainLoopRef();
    if (!loop.isLooping() || (getLength() <= 1)) {     // Don't kill a whole mute sound!
        return;
    }

    const auto loopStartIndex = loop.getStartIndex();
    const auto endIndex = loop.getEndIndex();
    jassert(endIndex == length - 1);       // No past-end cells must be present.
    const auto loopLength = loop.getLength();

    auto cellCountToRemove = 0;
    for (auto cellIndex = loopStartIndex; cellIndex <= endIndex; ++cellIndex) {
        if (psgPart.getCellRefConst(cellIndex).isMute()) {
            ++cellCountToRemove;
        } else {
            break;
        }
    }

    if (cellCountToRemove == loopLength) {
        // Mute section found. Optimization possible.
        if (loopLength < length) {
            psgPart.removeCells(length - cellCountToRemove, cellCountToRemove);
            psgPart.setMainLoop(Loop(0, endIndex - cellCountToRemove, false));
        } else {
            jassert(loopLength == length);
            // Deleted the whole sound!
            psgPart.removeCells(1, cellCountToRemove - 1);
            psgPart.setMainLoop(Loop(0, 0, false));
        }
    }
}


// BaseItemOptimizer method implementations.
// ============================================

Loop InstrumentOptimizer::getLoop() noexcept
{
    return getItem().getConstPsgPart().getMainLoopRef();
}

void InstrumentOptimizer::setLoop(const Loop& loop) noexcept
{
    getItem().getPsgPart().setMainLoop(loop);
}

int InstrumentOptimizer::getLength() noexcept
{
    return getItem().getPsgPart().getLength();
}

int InstrumentOptimizer::getSpeed() noexcept
{
    return getItem().getPsgPart().getSpeed();
}

void InstrumentOptimizer::setSpeed(const int speed) noexcept
{
    getItem().getPsgPart().setSpeed(speed);
}

void InstrumentOptimizer::removeCells(const int cellIndex, const int cellCountToRemove) noexcept
{
    getItem().getPsgPart().removeCells(cellIndex, cellCountToRemove);
}

PsgInstrumentCell InstrumentOptimizer::getCell(const int cellIndex) noexcept
{
    return getItem().getPsgPart().getCellRefConst(cellIndex);
}

bool InstrumentOptimizer::areCellsMusicallyEqual(const PsgInstrumentCell& left, const PsgInstrumentCell& right) noexcept
{
    return left.isMusicallyEqualTo(right);
}

}   // namespace arkostracker
