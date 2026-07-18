#include "LowLevelPsgInstrumentCell.h"

#include "../../../utils/PsgValues.h"

namespace arkostracker 
{

const LowLevelPsgInstrumentCell LowLevelPsgInstrumentCell::emptyCell = LowLevelPsgInstrumentCell();

LowLevelPsgInstrumentCell::LowLevelPsgInstrumentCell() noexcept :
        volume(0),
        noise(0),
        softwarePeriod(0),
        softwareArpeggio(0),
        softwarePitch(0),
        link(PsgInstrumentCellLink::noSoftNoHard),
        ratio(PsgValues::defaultRatio),
        hardwareEnvelope(PsgValues::defaultHardwareEnvelope),
        hardwarePeriod(0),
        hardwareArpeggio(0),
        hardwarePitch(0),
        retrig(false),

        sidActivated(false),
        sidCycleBalancePercent(PsgValues::defaultSidBalancePercent),
        sidLowShelfVolume(PsgValues::defaultSidLowShelfVolume),
        sidRatio(PsgValues::defaultSidRatio),
        sidArpeggio(PsgValues::defaultSidArpeggio),
        sidPitch(PsgValues::defaultSidPitch),

        hashcode(0)
{
    calculateHashcode();
}

LowLevelPsgInstrumentCell::LowLevelPsgInstrumentCell(const PsgInstrumentCellLink pLink, const int pVolume, const int pNoise, const int pSoftwarePeriod,
                                     const int pSoftwareArpeggio, const int pSoftwarePitch,
                                     const int pRatio, const int pHardwareEnvelope, const int pHardwarePeriod, const int pHardwareArpeggio,
                                     const int pHardwarePitch, const bool pRetrig,
                                     const bool pSidActivated, const int pSidCycleBalancePercent, const int pSidLowShelfVolume,
                                     const int pSidRatio, const int pSidArpeggio, const int pSidPitch) noexcept :
        volume(pVolume),
        noise(pNoise),
        softwarePeriod(pSoftwarePeriod),
        softwareArpeggio(pSoftwareArpeggio),
        softwarePitch(pSoftwarePitch),
        link(pLink),
        ratio(pRatio),
        hardwareEnvelope(PsgValues::convertEnvelopeCurveToAt(pHardwareEnvelope)),
        hardwarePeriod(pHardwarePeriod),
        hardwareArpeggio(pHardwareArpeggio),
        hardwarePitch(pHardwarePitch),
        retrig(pRetrig),

        sidActivated(pSidActivated),
        sidCycleBalancePercent(pSidCycleBalancePercent),
        sidLowShelfVolume(pSidLowShelfVolume),
        sidRatio(pSidRatio),
        sidArpeggio(pSidArpeggio),
        sidPitch(pSidPitch),

        hashcode(0)
{
    jassert((pNoise >= 0) && (pNoise <= 31));
    jassert((pVolume >= 0) && (pVolume <= 15));
    calculateHashcode();
}

const LowLevelPsgInstrumentCell& LowLevelPsgInstrumentCell::getEmptyCell() noexcept
{
    return emptyCell;
}

LowLevelPsgInstrumentCell LowLevelPsgInstrumentCell::buildSoftwareCell(const int newVolume, const int newNoise, const int newArpeggio, const int newPitch,
                                                       const bool newSound) noexcept
{
    jassert((newVolume >= 0) && (newVolume <= 15));
    jassert((newNoise >= 0) && (newNoise <= 31));

    LowLevelPsgInstrumentCell cell;
    cell.volume = newVolume;
    cell.noise = newNoise;
    cell.softwareArpeggio = newArpeggio;
    cell.softwarePitch = newPitch;
    cell.link = newSound ? PsgInstrumentCellLink::softOnly : PsgInstrumentCellLink::noSoftNoHard;

    cell.calculateHashcode();

    return cell;
}

int LowLevelPsgInstrumentCell::getVolume() const noexcept
{
    return volume;
}

int LowLevelPsgInstrumentCell::getNoise() const noexcept
{
    return noise;
}

PsgInstrumentCellLink LowLevelPsgInstrumentCell::getLink() const noexcept
{
    return link;
}

int LowLevelPsgInstrumentCell::getHardwareEnvelope() const noexcept
{
    return hardwareEnvelope;
}

bool LowLevelPsgInstrumentCell::isRetrig() const noexcept
{
    return retrig;
}

bool LowLevelPsgInstrumentCell::doesForceSoftwarePeriod() const noexcept
{
    return (softwarePeriod != 0);       // 0 means "automatic".
}

int LowLevelPsgInstrumentCell::getSoftwarePeriod() const noexcept
{
    return softwarePeriod;
}

int LowLevelPsgInstrumentCell::getSoftwareArpeggio() const noexcept
{
    return softwareArpeggio;
}

int LowLevelPsgInstrumentCell::getSoftwarePitch() const noexcept
{
    return softwarePitch;
}

int LowLevelPsgInstrumentCell::getRatio() const noexcept
{
    return ratio;
}

bool LowLevelPsgInstrumentCell::doesForceHardwarePeriod() const noexcept
{
    return (hardwarePeriod != 0);       // 0 means "automatic".
}

int LowLevelPsgInstrumentCell::getHardwarePeriod() const noexcept
{
    return hardwarePeriod;
}

int LowLevelPsgInstrumentCell::getHardwareArpeggio() const noexcept
{
    return hardwareArpeggio;
}

int LowLevelPsgInstrumentCell::getHardwarePitch() const noexcept
{
    return hardwarePitch;
}

bool LowLevelPsgInstrumentCell::isHardware() const noexcept
{
    return (link != PsgInstrumentCellLink::noSoftNoHard) && (link != PsgInstrumentCellLink::softOnly);
}

bool LowLevelPsgInstrumentCell::isEmpty() const noexcept
{
    return (link == PsgInstrumentCellLink::noSoftNoHard) && (volume == 0) && (noise == 0) && (softwarePeriod == 0) && (softwareArpeggio == 0) && (softwarePitch == 0)
           && (ratio == 0) && (hardwareEnvelope <= 8) && (hardwarePeriod == 0) && (hardwareArpeggio == 0) && (hardwarePitch == 0) && !retrig
           && !sidActivated;
}

bool LowLevelPsgInstrumentCell::isSidActivated() const noexcept
{
    return sidActivated;
}

int LowLevelPsgInstrumentCell::getSidBalancePercent() const noexcept
{
    return sidCycleBalancePercent;
}

int LowLevelPsgInstrumentCell::getSidLowShelfVolume() const noexcept
{
    return sidLowShelfVolume;
}

int LowLevelPsgInstrumentCell::getSidRatio() const noexcept
{
    return sidRatio;
}

int LowLevelPsgInstrumentCell::getSidArpeggio() const noexcept
{
    return sidArpeggio;
}

int LowLevelPsgInstrumentCell::getSidPitch() const noexcept
{
    return sidPitch;
}

bool LowLevelPsgInstrumentCell::operator<(const LowLevelPsgInstrumentCell& other) const noexcept
{
    return (hashcode < other.hashcode);
}

bool LowLevelPsgInstrumentCell::operator==(const LowLevelPsgInstrumentCell& other) const noexcept
{
    return (hashcode == other.hashcode);
}

bool LowLevelPsgInstrumentCell::operator!=(const LowLevelPsgInstrumentCell& other) const noexcept
{
    return !operator==(other);
}

size_t LowLevelPsgInstrumentCell::getHashcode() const noexcept
{
    return hashcode;
}

void LowLevelPsgInstrumentCell::calculateHashcode() noexcept
{
    size_t result = 17;

    result = (31 * result) + (volume & 0xf);
    result = (31 * result) + (noise & 0x1f);
    result = (31 * result) + (softwarePeriod & 0xfff);
    result = (31 * result) + (softwareArpeggio & 0xff);
    result = (31 * result) + (softwarePitch & 0xffff);

    result = (31 * result) + static_cast<size_t>(link);
    result = (31 * result) + (ratio & 7);

    result = (31 * result) + (hardwareEnvelope & 0xf);
    result = (31 * result) + (hardwarePeriod & 0xffff);
    result = (31 * result) + (hardwareArpeggio & 0xff);
    result = (31 * result) + (hardwarePitch & 0xffff);

    result = (31 * result) + (retrig ? 1 : 0);

    result = (31U * result) + (sidActivated ? 1 : 0);
    result = (31U * result) + static_cast<size_t>(sidCycleBalancePercent);
    result = (31U * result) + static_cast<size_t>(sidLowShelfVolume);
    result = (31U * result) + static_cast<size_t>(sidRatio);
    result = (31U * result) + static_cast<size_t>(sidArpeggio);
    result = (31U * result) + static_cast<size_t>(sidPitch);

    hashcode = result;
}

bool LowLevelPsgInstrumentCell::equalMusically(const LowLevelPsgInstrumentCell& other) const noexcept
{
    if ((link != other.link) || (noise != other.noise)) {
        return false;
    }

    // No soft no hard or SoftOnly: the volume is in common.
    if ((link == PsgInstrumentCellLink::noSoftNoHard) || (link == PsgInstrumentCellLink::softOnly)) {
        if (volume != other.volume) {
            return false;
        }
    }

    // Checks the software part.
    if ((link == PsgInstrumentCellLink::softOnly) || (link == PsgInstrumentCellLink::softToHard) || (link == PsgInstrumentCellLink::softAndHard)) {

        if (softwarePeriod != other.softwarePeriod) {
            return false;
        }
        // If there is no forced period, compares the Arps and Pitch only, but only in these conditions (if forced period, arpeggio/pitch is not used).
        if ((softwarePeriod == 0) && ((softwareArpeggio != other.softwareArpeggio) || (softwarePitch != other.softwarePitch))) {
            return false;
        }
    }

    // Checks the hardware part.
    if (PsgInstrumentCellLinkHelper::isHardware(link)) {
        if ((hardwareEnvelope != other.hardwareEnvelope) || (retrig != other.retrig)) {
            return false;
        }

        if (link == PsgInstrumentCellLink::softToHard) {
            // In Soft To Hard, only the hardware pitch matters.
            if (hardwarePitch != other.hardwarePitch) {
                return false;
            }
        } else {
            // All other hardware sounds.
            if (hardwarePeriod != other.hardwarePeriod) {
                return false;
            }

            // If there is no forced period, compares the Arps and Pitch only, but only in these conditions (if forced period, arpeggio/pitch are not used).
            if ((hardwarePeriod == 0) && ((hardwareArpeggio != other.hardwareArpeggio) || (hardwarePitch != other.hardwarePitch))) {
                return false;
            }
        }
    }

    // Checks the software pitch for HardToSoft.
    if ((link == PsgInstrumentCellLink::hardToSoft) && (softwarePitch != other.softwarePitch)) {
        return false;
    }
    // Checks the hardware pitch for SoftToHard.
    if ((link == PsgInstrumentCellLink::softToHard) && (hardwarePitch != other.hardwarePitch)) {
        return false;
    }

    // Ratio. Only for Link that go from Soft to Hard or the opposite.
    if ((link == PsgInstrumentCellLink::softToHard) || (link == PsgInstrumentCellLink::hardToSoft)) {
        if (ratio != other.ratio) {
            return false;
        }
    }

    // Compares the SID.
    if (sidActivated != other.sidActivated) {
        return false;
    }

    if (sidActivated) {
        if ((sidLowShelfVolume != other.sidLowShelfVolume) || (sidCycleBalancePercent != other.sidCycleBalancePercent) || (sidRatio != other.sidRatio)
            || (sidArpeggio != other.sidArpeggio) || (sidPitch != other.sidPitch)) {
            return false;
        }
    }

    return true;
}

}   // namespace arkostracker
