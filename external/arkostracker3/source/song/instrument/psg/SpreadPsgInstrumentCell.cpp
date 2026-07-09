#include "SpreadPsgInstrumentCell.h"

#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "../../cells/CellConstants.h"

namespace arkostracker 
{

const SpreadPsgInstrumentCell SpreadPsgInstrumentCell::emptyCell(PsgInstrumentCell::getEmptyPsgInstrumentCell(), {});

SpreadPsgInstrumentCell::SpreadPsgInstrumentCell(const PsgInstrumentCell& pCellWithSpread, std::set<PsgSection> pGeneratedSections) noexcept :
        cellWithSpread(pCellWithSpread),
        generatedSections(std::move(pGeneratedSections))
{
}

const SpreadPsgInstrumentCell& SpreadPsgInstrumentCell::getEmptyCell() noexcept
{
    return emptyCell;
}

const std::set<PsgSection>& SpreadPsgInstrumentCell::getGeneratedSectionsRef() const noexcept
{
    return generatedSections;
}

const PsgInstrumentCell& SpreadPsgInstrumentCell::getCellWithAutoSpreadAppliedRef() const noexcept
{
    return cellWithSpread;
}

LowLevelPsgInstrumentCell SpreadPsgInstrumentCell::buildLowLevelCell() const noexcept           // TODO TU this.
{
    // Extracts the data from the reference Cell.
    const auto link = cellWithSpread.getLink();
    const auto volume = cellWithSpread.getVolume();
    const auto noise = cellWithSpread.getNoise();
    const auto ratio = cellWithSpread.getRatio();
    const auto primaryPeriod = cellWithSpread.getPrimaryPeriod();
    const auto primaryArpeggioNoteInOctave = cellWithSpread.getPrimaryArpeggioNoteInOctave();
    const auto primaryArpeggioOctave = cellWithSpread.getPrimaryArpeggioOctave();
    const auto primaryArpeggio = primaryArpeggioNoteInOctave + primaryArpeggioOctave * CellConstants::noteCountInOctave;
    const auto primaryPitch = cellWithSpread.getPrimaryPitch();
    const auto secondaryPeriod = cellWithSpread.getSecondaryPeriod();
    const auto secondaryArpeggioNoteInOctave = cellWithSpread.getSecondaryArpeggioNoteInOctave();
    const auto secondaryArpeggioOctave = cellWithSpread.getSecondaryArpeggioOctave();
    const auto secondaryArpeggio = secondaryArpeggioNoteInOctave + secondaryArpeggioOctave * CellConstants::noteCountInOctave;
    const auto secondaryPitch = cellWithSpread.getSecondaryPitch();
    const auto hardwareEnvelope = cellWithSpread.getHardwareEnvelope();
    const auto retrig = cellWithSpread.isRetrig();
    const auto isSidActivated = cellWithSpread.isSidActivated();
    const auto sidRatio = cellWithSpread.getSidRatio();
    const auto sidBalancePercent = cellWithSpread.getSidBalancePercent();
    const auto sidLowShelfVolume = cellWithSpread.getSidLowShelfVolume();
    const auto sidArpeggio = cellWithSpread.getSidArpeggio();
    const auto sidPitch = cellWithSpread.getSidPitch();

    int softwarePeriod;     // NOLINT(*-init-variables)
    int softwareArpeggio;   // NOLINT(*-init-variables)
    int softwarePitch;      // NOLINT(*-init-variables)
    int hardwarePeriod;     // NOLINT(*-init-variables)
    int hardwareArpeggio;   // NOLINT(*-init-variables)
    int hardwarePitch;      // NOLINT(*-init-variables)

    // Software or hardware ? If Soft is basis, we consider primary = soft.
    if (cellWithSpread.isSoftware() || (link == PsgInstrumentCellLink::softToHard) || (link == PsgInstrumentCellLink::softAndHard)) {
        softwarePeriod = NumberUtil::correctNumber(primaryPeriod, PsgValues::minimumPeriod, PsgValues::maximumSoftwarePeriod);
        softwareArpeggio = primaryArpeggio;
        softwarePitch = primaryPitch;
        hardwarePeriod = NumberUtil::correctNumber(secondaryPeriod, PsgValues::minimumPeriod, PsgValues::maximumHardwarePeriod);
        hardwareArpeggio = secondaryArpeggio;
        hardwarePitch = secondaryPitch;
    } else {
        // Hardware-basis (Hard to Soft, Hard only).
        jassert((link == PsgInstrumentCellLink::hardOnly) || (link == PsgInstrumentCellLink::hardToSoft));

        hardwarePeriod = NumberUtil::correctNumber(primaryPeriod, PsgValues::minimumPeriod, PsgValues::maximumHardwarePeriod);
        hardwareArpeggio = primaryArpeggio;
        hardwarePitch = primaryPitch;
        softwarePeriod = NumberUtil::correctNumber(secondaryPeriod, PsgValues::minimumPeriod, PsgValues::maximumSoftwarePeriod);
        softwareArpeggio = secondaryArpeggio;
        softwarePitch = secondaryPitch;
    }

    return { link, volume, noise,
             softwarePeriod, softwareArpeggio, softwarePitch,
             ratio, hardwareEnvelope,
             hardwarePeriod, hardwareArpeggio, hardwarePitch, retrig,
             isSidActivated, sidBalancePercent, sidLowShelfVolume, sidRatio, sidArpeggio, sidPitch
    };
}

}   // namespace arkostracker
