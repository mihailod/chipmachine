#include "PsgInstrumentEditorControllerLogic.h"

#include "../../../../controllers/SongController.h"
#include "ValueInterpreter.h"

namespace arkostracker 
{

std::unique_ptr<PsgInstrumentCell> PsgInstrumentEditorControllerLogic::checkValueAndChangeInstrumentCell(const AreaType areaType, const int targetBarIndex, const int value,
                                                                                                         const SongController& songController, const Id& instrumentId) noexcept
{
    // Finds the cell to get the data from. It should be the selected bar of course, but if the user picks an out-of-bound bar, we will select
    // the last possible one. The goal is to spread it all the way if Setting after the last bar, which is quite handy to the user.
    PsgInstrumentCell originalCell;
    bool autoSpreadHere; // NOLINT(*-init-variables)
    int length;          // NOLINT(*-init-variables)
    bool outOfBounds;    // NOLINT(*-init-variables)
    songController.performOnConstInstrument(instrumentId, [&](const Instrument& instrument) noexcept {
        const auto& psgPart = instrument.getConstPsgPart();
        length = psgPart.getLength();

        // Gets the Cell. If out of bounds, picks the last cell.
        outOfBounds = (targetBarIndex >= length);
        const auto barIndex = outOfBounds ? (length - 1) : targetBarIndex;

        // IMPORTANT NOTE: This works because we don't allow modifying the auto-spread values. Else, we would need to get the spread value for comparison, but
        // use the original cell value as reference for the other fields (would need to pass both cells to the ValueInterpreter*s* below).
        originalCell = psgPart.getCellRefConst(barIndex);           // Can handle out of bounds.

        autoSpreadHere = psgPart.isGeneratedValueAtIndex(convertAreaType(areaType), barIndex);
    });

    const auto forceGeneration = outOfBounds;

    // Don't allow modifying a cell that is generated, the user wouldn't see it directly.
    // Exception: if out of bounds, do it anyway (mandatory, else nothing would ever be done if writing on the same line!).
    if (autoSpreadHere && !outOfBounds) {
        return nullptr;
    }

    // Generates a new cell, returns null if no change.

    std::unique_ptr<PsgInstrumentCell> newCell;
    switch (areaType) {
        default:
            jassertfalse;
            break;
        case AreaType::soundType:
            newCell = ValueInterpreter::createCellForTypeChange(originalCell, value, forceGeneration);
            break;
        case AreaType::envelope:
            newCell = ValueInterpreter::createCellForEnvelopeChange(originalCell, value, forceGeneration);
            break;
        case AreaType::noise:
            newCell = ValueInterpreter::createCellForNoiseChange(originalCell, value, forceGeneration);
            break;
        case AreaType::primaryPeriod:
            newCell = ValueInterpreter::createCellForPrimaryPeriodChange(originalCell, value, forceGeneration);
            break;
        case AreaType::primaryArpeggioNoteInOctave:
            newCell = ValueInterpreter::createCellForPrimaryArpeggioNoteInOctave(originalCell, value, forceGeneration);
            break;
        case AreaType::primaryArpeggioOctave:
            newCell = ValueInterpreter::createCellForPrimaryArpeggioOctave(originalCell, value, forceGeneration);
            break;
        case AreaType::primaryPitch:
            newCell = ValueInterpreter::createCellForPrimaryPitch(originalCell, value, forceGeneration);
            break;
        case AreaType::secondaryPeriod:
            newCell = ValueInterpreter::createCellForSecondaryPeriodChange(originalCell, value, forceGeneration);
            break;
        case AreaType::secondaryArpeggioNoteInOctave:
            newCell = ValueInterpreter::createCellForSecondaryArpeggioNoteInOctave(originalCell, value, forceGeneration);
            break;
        case AreaType::secondaryArpeggioOctave:
            newCell = ValueInterpreter::createCellForSecondaryArpeggioOctave(originalCell, value, forceGeneration);
            break;
        case AreaType::secondaryPitch:
            newCell = ValueInterpreter::createCellForSecondaryPitch(originalCell, value, forceGeneration);
            break;
        case AreaType::sidIsActivatedAndRatio:
            newCell = ValueInterpreter::createCellForSidIsActivatedAndRatio(originalCell, value, forceGeneration);
            break;
        case AreaType::sidBalancePercent:
            newCell = ValueInterpreter::createCellForSidBalancePercent(originalCell, value, forceGeneration);
            break;
        case AreaType::sidLowShelfVolume:
            newCell = ValueInterpreter::createCellForSidLowShelfVolume(originalCell, value, forceGeneration);
            break;
        case AreaType::sidArpeggio:
            newCell = ValueInterpreter::createCellForSidArpeggio(originalCell, value, forceGeneration);
            break;
        case AreaType::sidPitch:
            newCell = ValueInterpreter::createCellForSidPitch(originalCell, value, forceGeneration);
            break;
    }

    return newCell;
}

PsgSection PsgInstrumentEditorControllerLogic::convertAreaType(const AreaType areaType) noexcept
{
    // The mapping is quite simple...
    switch (areaType) {
        default:
            jassertfalse;               // Should never happen!
        case AreaType::soundType:
            return PsgSection::soundType;
        case AreaType::envelope:
            return PsgSection::envelope;
        case AreaType::noise:
            return PsgSection::noise;
        case AreaType::primaryPeriod:
            return PsgSection::primaryPeriod;
        case AreaType::primaryArpeggioNoteInOctave:
            return PsgSection::primaryArpeggioNoteInOctave;
        case AreaType::primaryArpeggioOctave:
            return PsgSection::primaryArpeggioOctave;
        case AreaType::primaryPitch:
            return PsgSection::primaryPitch;
        case AreaType::secondaryPeriod:
            return PsgSection::secondaryPeriod;
        case AreaType::secondaryArpeggioNoteInOctave:
            return PsgSection::secondaryArpeggioNoteInOctave;
        case AreaType::secondaryArpeggioOctave:
            return PsgSection::secondaryArpeggioOctave;
        case AreaType::secondaryPitch:
            return PsgSection::secondaryPitch;

        case AreaType::sidBalancePercent:
            return PsgSection::sidBalancePercent;
        case AreaType::sidLowShelfVolume:
            return PsgSection::sidLowShelfVolume;
        case AreaType::sidIsActivatedAndRatio:
            return PsgSection::sidIsActivated;
        case AreaType::sidArpeggio:
            return PsgSection::sidArpeggio;
        case AreaType::sidPitch:
            return PsgSection::sidPitch;
    }
}

}   // namespace arkostracker
