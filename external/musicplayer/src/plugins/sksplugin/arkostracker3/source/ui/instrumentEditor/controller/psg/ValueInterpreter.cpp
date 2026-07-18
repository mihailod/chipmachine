#include "ValueInterpreter.h"

#include "../../../../song/cells/CellConstants.h"
#include "../../../../utils/CollectionUtil.h"
#include "../../../../utils/NumberUtil.h"
#include "../../../../utils/PsgValues.h"
#include "ViewedValueLimits.h"

namespace arkostracker 
{

const std::unordered_map<PsgInstrumentCellLink, int> ValueInterpreter::linkToBarValue = { // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
        { PsgInstrumentCellLink::noSoftNoHard, 0 },
        { PsgInstrumentCellLink::softOnly,     1 },
        { PsgInstrumentCellLink::softToHard,   2 },
        { PsgInstrumentCellLink::hardOnly,     3 },
        { PsgInstrumentCellLink::hardToSoft,   4 },
        { PsgInstrumentCellLink::softAndHard,  5 }
};

const std::unordered_map<int, int> ValueInterpreter::barValueEnvelopeToValidEnvelope = { // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
        { 0, 0x8 },
        { 1, 0xa },
        { 2, 0xc },
        { 3, 0xe },
        { 4, 0x9 },
        { 5, 0xb },
        { 6, 0xd },
        { 7, 0xf }
};

int ValueInterpreter::getLinkToBarValue(const PsgInstrumentCellLink link) noexcept
{
    return CollectionUtil::findAndGet(linkToBarValue, link, 0, true);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForTypeChange(const PsgInstrumentCell& originalCell, int inputValue, const bool forceGeneration) noexcept
{
    // Checks the limit first.
    inputValue = NumberUtil::correctNumber(inputValue, static_cast<int>(PsgInstrumentCellLink::first), static_cast<int>(PsgInstrumentCellLink::last));

    // Finds the matching entry. Not efficient... Who cares.
    const auto optionalLink = CollectionUtil::findKeyFromValue(linkToBarValue, inputValue);
    PsgInstrumentCellLink link; // NOLINT(*-init-variables)
    if (optionalLink.isAbsent()) {
        jassertfalse;       // Abnormal!
        link = PsgInstrumentCellLink::noSoftNoHard;
    } else {
        link = optionalLink.getValue();
    }

    // Same value? If yes, nothing to do.
    if (!forceGeneration && (originalCell.getLink() == link)) {
        return nullptr;
    }

    // Creates a new Cell with the new link.
    const auto newCell = originalCell.withLink(link);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForNoiseChange(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    const auto noise = NumberUtil::correctNumber(inputValue, PsgValues::minimumNoise, PsgValues::maximumNoise);

    // Any change?
    if (!forceGeneration && (noise == originalCell.getNoise())) {
        return nullptr;
    }

    // Creates a new Cell with the new noise.
    const auto newCell = originalCell.withNoise(noise);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForEnvelopeChange(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    // Software or hardware?
    if (!originalCell.isHardware()) {
        // Software. The value is thus a volume.
        const auto volume = NumberUtil::correctNumber(inputValue, PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);
        // Any change?
        if (!forceGeneration && (volume == originalCell.getVolume())) {
            return nullptr;
        }

        const auto newCell = originalCell.withVolume(volume);
        return std::make_unique<PsgInstrumentCell>(newCell);
    }

    // Hardware.
    // In order, curves that are the most used.
    // 8, a, c, e, then 9, b, d, f, with each 8 ratio each.
    const auto newValue = NumberUtil::correctNumber(inputValue, ViewedValueLimits::hardwareEnvelopeBarMinimumAllValue,
        ViewedValueLimits::hardwareEnvelopeBarMaximumAllValue);

    // Converts the value to envelope/ratio.
    const auto rawNewEnvelope = newValue / ViewedValueLimits::ratioCount;          // Starts at 0!
    const auto newRatio = newValue % ViewedValueLimits::ratioCount;
    // Maps the raw envelope into the ones we want first.
    const auto newEnvelope = CollectionUtil::findAndGet(barValueEnvelopeToValidEnvelope, rawNewEnvelope, PsgValues::minimumHardwareEnvelope,
                                                        true);

    // Any change?
    if (!forceGeneration && ((originalCell.getHardwareEnvelope() == newEnvelope) && (originalCell.getRatio() == newRatio))) {
        return nullptr;
    }

    // Creates a new Cell with the new envelope/ratio.
    const auto newCell = originalCell.withEnvelopeAndRatio(newEnvelope, newRatio);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

int ValueInterpreter::determineEnvelopeUiValueFromCell(const PsgInstrumentCell& cell) noexcept
{
    const auto isHardware = cell.isHardware();
    // If software, the volume is simply returned.
    if (!isHardware) {
        return cell.getVolume();
    }

    // Hardware.
    const auto envelope = cell.getHardwareEnvelope();
    const auto ratio = cell.getRatio();
    // The envelope is mapped into another order (most used first).
    const auto uiEnvelopeOptional = CollectionUtil::findKeyFromValue(barValueEnvelopeToValidEnvelope, envelope);
    int uiEnvelope; // NOLINT(*-init-variables)
    if (uiEnvelopeOptional.isAbsent()) {
        // Abnormal!!
        uiEnvelope = 0;
        jassertfalse;
    } else {
        uiEnvelope = uiEnvelopeOptional.getValue();
    }

    // Interlaces the envelope and the ratio.
    return (uiEnvelope * ViewedValueLimits::ratioCount) + ratio;
}

bool ValueInterpreter::isMostUsefulHardwareEnvelope(const int hardwareEnvelope) noexcept
{
    const auto uiEnvelopeOptional = CollectionUtil::findKeyFromValue(barValueEnvelopeToValidEnvelope, hardwareEnvelope);
    if (uiEnvelopeOptional.isAbsent()) {
        // Abnormal!!
        jassertfalse;
        return false;
    }

    return uiEnvelopeOptional.getValue() < ViewedValueLimits::mostlyUsedEnvelopesCount;
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPrimaryPeriodChange(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForPeriodChange(originalCell, inputValue,
                              [](const PsgInstrumentCell& pOriginalCell) noexcept -> int {
                                  return pOriginalCell.getPrimaryPeriod();
                              },
                              [](const PsgInstrumentCell& pOriginalCell, const int pPeriod) noexcept -> PsgInstrumentCell {
                                  return pOriginalCell.withPrimaryPeriod(pPeriod);
                              }, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPrimaryArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForArpeggioNoteInOctave(originalCell, inputValue, true, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPrimaryArpeggioOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForArpeggioOctave(originalCell, inputValue, true, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPrimaryPitch(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForPitch(originalCell, inputValue, true, forceGeneration);
}


// -----------------------------------------------------------------

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSecondaryPeriodChange(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForPeriodChange(originalCell, inputValue,
                              [](const PsgInstrumentCell& pOriginalCell) noexcept -> int {
                                  return pOriginalCell.getSecondaryPeriod();
                              },
                              [](const PsgInstrumentCell& pOriginalCell, const int pPeriod) noexcept -> PsgInstrumentCell {
                                  return pOriginalCell.withSecondaryPeriod(pPeriod);
                              }, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSecondaryArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForArpeggioNoteInOctave(originalCell, inputValue, false, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSecondaryArpeggioOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForArpeggioOctave(originalCell, inputValue, false, forceGeneration);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSecondaryPitch(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    return createCellForPitch(originalCell, inputValue, false, forceGeneration);
}


// -----------------------------------------------------------------

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSidIsActivatedAndRatio(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    // Input to -1 means "not activated".
    const auto isActivated = (inputValue >= 0);
    const auto ratio = std::min(isActivated ? inputValue : PsgValues::sidMinimumRatio, PsgValues::sidMaximumRatio);

    const auto originalIsActivated = originalCell.isSidActivated();
    const auto originalRatio = originalCell.getSidRatio();
    if (!forceGeneration && (isActivated == originalIsActivated) && (ratio == originalRatio)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = originalCell
        .withSidActivated(isActivated)
        .withSidRatio(ratio);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSidBalancePercent(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    const auto percent = NumberUtil::correctNumber(inputValue, 0, 100);

    const auto originalPercent = originalCell.getSidBalancePercent();
    if (!forceGeneration && (percent == originalPercent)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = originalCell.withSidBalancePercent(percent);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSidLowShelfVolume(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    const auto volume = NumberUtil::correctNumber(inputValue, PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);

    const auto originalVolume = originalCell.getSidLowShelfVolume();
    if (!forceGeneration && (volume == originalVolume)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = originalCell.withSidLowShelfVolume(volume);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSidArpeggio(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    const auto arpeggio = NumberUtil::correctNumber(inputValue, PsgValues::sidMinimumArpeggio, PsgValues::sidMaximumArpeggio);

    const auto originalArpeggio = originalCell.getSidArpeggio();
    if (!forceGeneration && (arpeggio == originalArpeggio)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = originalCell.withSidArpeggio(arpeggio);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForSidPitch(const PsgInstrumentCell& originalCell, const int inputValue, const bool forceGeneration) noexcept
{
    const auto pitch = NumberUtil::correctNumber(inputValue, PsgValues::sidMinimumPitch, PsgValues::sidMaximumPitch);

    const auto originalPitch = originalCell.getSidPitch();
    if (!forceGeneration && (pitch == originalPitch)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = originalCell.withSidPitch(pitch);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

// -----------------------------------------------------------------

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPeriodChange(const PsgInstrumentCell& originalCell, const int inputValue,
        const std::function<int(const PsgInstrumentCell&)>& getPeriodLambda,
        const std::function<PsgInstrumentCell(const PsgInstrumentCell&, int)>& createCellLambda, const bool forceGeneration) noexcept
{
    // Lambda are used... Probably overkill. Later I use only a simple boolean.
    const auto period = NumberUtil::correctNumber(inputValue, PsgValues::minimumPeriod, PsgValues::maximumPeriod);

    // Any change?
    const auto originalPeriod = getPeriodLambda(originalCell);
    if (!forceGeneration && (period == originalPeriod)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = createCellLambda(originalCell, period);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForArpeggioNoteInOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool primary, const bool forceGeneration) noexcept
{
    const auto newNoteInOctave = NumberUtil::correctNumber(inputValue, CellConstants::firstNoteInOctave, CellConstants::lastNoteInOctave);

    // Any change?
    const auto originalArpeggioNoteInOctave = primary ? originalCell.getPrimaryArpeggioNoteInOctave() : originalCell.getSecondaryArpeggioNoteInOctave();
    if (!forceGeneration && (newNoteInOctave == originalArpeggioNoteInOctave)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = primary ? originalCell.withPrimaryArpeggioNoteInOctave(newNoteInOctave) : originalCell.withSecondaryArpeggioNoteInOctave(newNoteInOctave);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForArpeggioOctave(const PsgInstrumentCell& originalCell, const int inputValue, const bool primary, const bool forceGeneration) noexcept
{
    const auto newOctave = NumberUtil::correctNumber(inputValue, PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave);

    // Any change?
    const auto originalArpeggioOctave = primary ? originalCell.getPrimaryArpeggioOctave() : originalCell.getSecondaryArpeggioOctave();
    if (!forceGeneration && (newOctave == originalArpeggioOctave)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = primary ? originalCell.withPrimaryArpeggioOctave(newOctave) : originalCell.withSecondaryArpeggioOctave(newOctave);
    return std::make_unique<PsgInstrumentCell>(newCell);
}

std::unique_ptr<PsgInstrumentCell> ValueInterpreter::createCellForPitch(const PsgInstrumentCell& originalCell, const int inputValue, const bool primary, const bool forceGeneration) noexcept
{
    const auto pitch = NumberUtil::correctNumber(inputValue, PsgValues::minimumPitch, PsgValues::maximumPitch);

    const auto originalPitch = primary ? originalCell.getPrimaryPitch() : originalCell.getSecondaryPitch();
    if (!forceGeneration && (pitch == originalPitch)) {
        return nullptr;
    }

    // Creates a new Cell with the new value.
    const auto newCell = primary ? originalCell.withPrimaryPitch(pitch) : originalCell.withSecondaryPitch(pitch);
    return std::make_unique<PsgInstrumentCell>(newCell);
}


}   // namespace arkostracker

