#include "PsgPart.h"

#include "../../../utils/NumberUtil.h"
#include "PsgPartConstants.h"

namespace arkostracker 
{

PsgPart::PsgPart() noexcept :
        speed(0),
        instrumentRetrig(false),
        cells(),
        sectionToAutoSpreadLoop(),
        soundEffectExported(true)           // By default, exported.
{
    // Fills the auto-spread loops with a default loop.
    const Loop defaultLoop;
    jassert(!defaultLoop.isLooping());
    jassert(defaultLoop.getStartIndex() == 0);
    jassert(defaultLoop.getEndIndex() == 0);
    for (const auto psgSection : getAllSections()) {
        setAutoSpreadLoop(psgSection, defaultLoop);
    }
}

std::vector<PsgSection> PsgPart::getAllSections() noexcept
{
    static const std::vector psgSections = {
            PsgSection::soundType, PsgSection::envelope, PsgSection::noise,
            PsgSection::primaryPeriod, PsgSection::primaryArpeggioNoteInOctave, PsgSection::primaryArpeggioOctave, PsgSection::primaryPitch,
            PsgSection::secondaryPeriod, PsgSection::secondaryArpeggioNoteInOctave, PsgSection::secondaryArpeggioOctave, PsgSection::secondaryPitch,
            PsgSection::sidIsActivated, PsgSection::sidBalancePercent, PsgSection::sidLowShelfVolume, PsgSection::sidArpeggio, PsgSection::sidPitch
    };
    jassert(psgSections.size() == static_cast<size_t>(PsgSection::count));

    return psgSections;
}

int PsgPart::getSpeed() const noexcept
{
    return speed;
}

void PsgPart::setSpeed(const int newSpeed) noexcept
{
    jassert((newSpeed >= 0) && (newSpeed <= PsgPartConstants::slowestSpeed));
    speed = newSpeed;
}

void PsgPart::addCell(const PsgInstrumentCell& cell) noexcept
{
    cells.push_back(cell);
}

void PsgPart::insertCell(const int cellIndex, const PsgInstrumentCell& cell) noexcept
{
    jassert(cellIndex <= static_cast<int>(cells.size()));
    cells.insert(cells.begin() + cellIndex, cell);
}

void PsgPart::removeCells(const int cellIndex, const int cellCount) noexcept
{
    jassert(cellCount >= 0);
    if (cellCount <= 0) {
        return;
    }

    jassert(cellIndex >= 0);
    jassert(cellIndex < static_cast<int>(cells.size()));
    jassert(cellCount > 0);
    jassert((cellIndex + cellCount) <= static_cast<int>(cells.size()));

    cells.erase(cells.begin() + cellIndex, cells.begin() + cellIndex + cellCount);
}

void PsgPart::setInstrumentRetrig(const bool newRetrig) noexcept
{
    instrumentRetrig = newRetrig;
}

bool PsgPart::isInstrumentRetrig() const noexcept
{
    return instrumentRetrig;
}

const PsgInstrumentCell& PsgPart::getCellRefConst(const int cellIndex) const noexcept
{
    jassert(cellIndex >= 0);

    // Within bounds.
    if (cellIndex < static_cast<int>(cells.size())) {
        return cells.at(static_cast<size_t>(cellIndex));
    }

    // Out of bounds!
    return PsgInstrumentCell::getEmptyPsgInstrumentCell();
}

void PsgPart::performOnEachCell(const std::function<void(int, const PsgInstrumentCell&)>& function) const noexcept
{
    auto index = 0;
    for (const auto& cell : cells) {
        function(index, cell);
        ++index;
    }
}

int PsgPart::getLength() const noexcept
{
    return static_cast<int>(cells.size());
}

Loop PsgPart::getMainLoop() const noexcept
{
    return mainLoop;
}

const Loop& PsgPart::getMainLoopRef() const noexcept
{
    return mainLoop;
}

void PsgPart::setMainLoop(const Loop& loop) noexcept
{
    mainLoop = loop;
}

void PsgPart::setCell(const int cellIndex, const PsgInstrumentCell& cell) noexcept
{
    // Valid index?
    if ((cellIndex < 0) || (cellIndex >= getLength())) {
        jassertfalse;
        return;
    }

    cells.at(static_cast<size_t>(cellIndex)) = cell;
}

SpreadPsgInstrumentCell PsgPart::buildSpreadPsgInstrumentCell(const int cellIndex) const noexcept
{
    // Valid index?
    if ((cellIndex < 0) || (cellIndex >= getLength())) {
        jassertfalse;
        return SpreadPsgInstrumentCell::getEmptyCell();
    }

    // As an optimization, gathers all the cells we need here, because we may require several because of the auto-spread.
    std::unordered_map<int, PsgInstrumentCell> cellIndexToCell;
    std::unordered_set<int> cellIndexesToGet;

    const auto& originalCell = getCellRefConst(cellIndex);

    // Gets all the auto-spread loops, as well as all the Cell indexes we will need to read from.
    for (const auto& entry : sectionToAutoSpreadLoop) {
        const auto& loop = entry.second;
        if (loop.isLooping()) {
            for (auto index = loop.getStartIndex(), endIndex = loop.getEndIndex(); index <= endIndex; ++index) {
                cellIndexesToGet.insert(index);
            }
        }
    }
    // Now we have the cell indexes, gets the cells themselves.
    for (const auto cellIndexToGet : cellIndexesToGet) {
        const auto& cellToGet = getCellRefConst(cellIndexToGet);
        cellIndexToCell.insert(std::make_pair(cellIndexToGet, cellToGet));
    }

    std::set<PsgSection> generatedSections;           // Filled by the calls below.
    const auto newLink = determineValueInCellSlot<PsgInstrumentCellLink>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop, PsgSection::soundType,
                                                                         generatedSections, [&](const PsgInstrumentCell& cell) noexcept { return cell.getLink(); });
    const auto newVolume = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop, PsgSection::envelope, generatedSections,
                                                         [&](const PsgInstrumentCell& cell) noexcept { return cell.getVolume(); });
    const auto newNoise = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop, PsgSection::noise, generatedSections,
                                                        [&](const PsgInstrumentCell& cell) noexcept { return cell.getNoise(); });

    const auto newPrimaryPeriod = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop, PsgSection::primaryPeriod,
                                                                generatedSections, [&](const PsgInstrumentCell& cell) noexcept { return cell.getPrimaryPeriod(); });
    const auto newPrimaryArpeggioNoteInOctave = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                              PsgSection::primaryArpeggioNoteInOctave, generatedSections,
                                                                              [&](const PsgInstrumentCell& cell) noexcept { return cell.getPrimaryArpeggioNoteInOctave(); });
    const auto newPrimaryArpeggioOctave = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                        PsgSection::primaryArpeggioOctave, generatedSections,
                                                                        [&](const PsgInstrumentCell& cell) noexcept { return cell.getPrimaryArpeggioOctave(); });
    const auto newPrimaryPitch = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                               PsgSection::primaryPitch, generatedSections,
                                                               [&](const PsgInstrumentCell& cell) noexcept { return cell.getPrimaryPitch(); });

    // The ratio, hardware envelope and retrig are linked to the auto-spread of the envelope.
    const auto newRatio = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                        PsgSection::envelope, generatedSections, [&](const PsgInstrumentCell& cell) noexcept { return cell.getRatio(); });
    const auto newHardwareEnvelope = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                   PsgSection::envelope, generatedSections,
                                                                   [&](const PsgInstrumentCell& cell) noexcept { return cell.getHardwareEnvelope(); });
    const auto newRetrig = determineValueInCellSlot<bool>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                          PsgSection::envelope, generatedSections, [&](const PsgInstrumentCell& cell) noexcept { return cell.isRetrig(); });

    const auto newSecondaryPeriod = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop, PsgSection::secondaryPeriod,
                                                                  generatedSections, [&](const PsgInstrumentCell& cell) noexcept { return cell.getSecondaryPeriod(); });
    const auto newSecondaryArpeggioNoteInOctave = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                                PsgSection::secondaryArpeggioNoteInOctave, generatedSections,
                                                                                [&](const PsgInstrumentCell& cell) noexcept { return cell.getSecondaryArpeggioNoteInOctave(); });
    const auto newSecondaryArpeggioOctave = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                          PsgSection::secondaryArpeggioOctave, generatedSections,
                                                                          [&](const PsgInstrumentCell& cell) noexcept { return cell.getSecondaryArpeggioOctave(); });
    const auto newSecondaryPitch = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::secondaryPitch, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept { return cell.getSecondaryPitch(); });

    const auto newSidCycleBalancePercent = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::sidBalancePercent, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept { return cell.getSidBalancePercent(); });
    const auto newSidLowShelfVolume = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::sidLowShelfVolume, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept { return cell.getSidLowShelfVolume(); });
    const auto [newIsSidActivated, newSidRatio] = determineValueInCellSlot<std::pair<bool, int>>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::sidIsActivated, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept {
                                                                     return std::make_pair(cell.isSidActivated(), cell.getSidRatio());
                                                                 });
    const auto newSidArpeggio = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::sidArpeggio, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept {
                                                                     return cell.getSidArpeggio();
                                                                 });
    const auto newSidPitch = determineValueInCellSlot<int>(cellIndex, originalCell, cellIndexToCell, sectionToAutoSpreadLoop,
                                                                 PsgSection::sidPitch, generatedSections,
                                                                 [&](const PsgInstrumentCell& cell) noexcept {
                                                                     return cell.getSidPitch();
                                                                 });

    const PsgInstrumentCell cell(newLink, newVolume, newNoise, newPrimaryPeriod, newPrimaryArpeggioNoteInOctave, newPrimaryArpeggioOctave,
                           newPrimaryPitch, newRatio, newSecondaryPeriod, newSecondaryArpeggioNoteInOctave, newSecondaryArpeggioOctave, newSecondaryPitch,
                           newHardwareEnvelope, newRetrig,
                           newIsSidActivated, newSidCycleBalancePercent, newSidLowShelfVolume, newSidRatio, newSidArpeggio, newSidPitch
    );

    return { cell, generatedSections };
}

LowLevelPsgInstrumentCell PsgPart::buildLowLevelCell(const int cellIndex) const noexcept
{
    const auto spreadCell = buildSpreadPsgInstrumentCell(cellIndex);
    return spreadCell.buildLowLevelCell();
}

bool PsgPart::isAutoSpread(const PsgSection psgSection) const noexcept
{
    return getAutoSpreadLoopRef(psgSection).isLooping();
}

Loop PsgPart::getAutoSpreadLoop(const PsgSection psgSection) const noexcept
{
    return sectionToAutoSpreadLoop.at(psgSection);
}

const Loop& PsgPart::getAutoSpreadLoopRef(const PsgSection psgSection) const noexcept
{
    return sectionToAutoSpreadLoop.at(psgSection);
}

void PsgPart::setAutoSpreadLoop(const PsgSection psgSection, const Loop& autoSpreadLoop) noexcept
{
    sectionToAutoSpreadLoop[psgSection] = autoSpreadLoop;
}

std::unordered_map<PsgSection, Loop> PsgPart::getAutoSpreadAllLoops() const noexcept
{
    return sectionToAutoSpreadLoop;
}

void PsgPart::setAutoSpreadLoops(const std::unordered_map<PsgSection, Loop>& newLoops) noexcept
{
    for (const auto& [psgSection, loop] : newLoops) {
        sectionToAutoSpreadLoop[psgSection] = loop;
    }
}

bool PsgPart::isGeneratedValueAtIndex(const PsgSection psgSection, const int cellIndex) const noexcept
{
    const auto& loop = getAutoSpreadLoopRef(psgSection);
    return (loop.isLooping() && (cellIndex > loop.getEndIndex()));
}

bool PsgPart::isSoundEffectExported() const noexcept
{
    return soundEffectExported;
}

void PsgPart::setSoundEffectExported(const bool exported) noexcept
{
    soundEffectExported = exported;
}

bool PsgPart::operator==(const PsgPart& other) const noexcept
{
    return (speed == other.speed)
        && (instrumentRetrig == other.instrumentRetrig)
        && (cells == other.cells)
        && (mainLoop == other.mainLoop)
        && (sectionToAutoSpreadLoop == other.sectionToAutoSpreadLoop)
        && (soundEffectExported == other.soundEffectExported);
}

bool PsgPart::operator!=(const PsgPart& other) const noexcept
{
    return !(other == *this);
}

}   // namespace arkostracker
