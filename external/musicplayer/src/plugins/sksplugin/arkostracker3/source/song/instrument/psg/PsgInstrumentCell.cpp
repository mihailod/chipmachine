#include "PsgInstrumentCell.h"

#include "../../../song/cells/CellConstants.h"
#include "../../../utils/PsgValues.h"

namespace arkostracker 
{

const PsgInstrumentCell PsgInstrumentCell::emptyCell = PsgInstrumentCell(); // NOLINT(*-statically-constructed-objects)

PsgInstrumentCell::PsgInstrumentCell() noexcept :
        volume(0),
        noise(0),
        primaryPeriod(0),
        primaryArpeggioNoteInOctave(0),
        primaryArpeggioOctave(0),
        primaryPitch(0),

        link(PsgInstrumentCellLink::noSoftNoHard),
        ratio(PsgValues::defaultRatio),
        hardwareEnvelope(PsgValues::defaultHardwareEnvelope),

        secondaryPeriod(0),
        secondaryArpeggioNoteInOctave(0),
        secondaryArpeggioOctave(0),
        secondaryPitch(0),

        retrig(false),

        sidActivated(false),
        sidCycleBalancePercent(PsgValues::defaultSidBalancePercent),
        sidLowShelfVolume(PsgValues::defaultSidLowShelfVolume),
        sidRatio(PsgValues::defaultSidRatio),
        sidArpeggio(PsgValues::defaultSidArpeggio),
        sidPitch(PsgValues::defaultSidPitch),

        hashcode(0U)
{
    jassert(primaryArpeggioNoteInOctave < CellConstants::noteCountInOctave);
    jassert((primaryArpeggioOctave >= PsgValues::minimumArpeggioOctave) && (primaryArpeggioOctave <= PsgValues::maximumArpeggioOctave));
    jassert(secondaryArpeggioNoteInOctave < CellConstants::noteCountInOctave);
    jassert((secondaryArpeggioOctave >= PsgValues::minimumArpeggioOctave) && (secondaryArpeggioOctave <= PsgValues::maximumArpeggioOctave));

    calculateHashcode();
}

PsgInstrumentCell::PsgInstrumentCell(const PsgInstrumentCellLink pLink, const int pVolume, const int pNoise, const int pPrimaryPeriod,
                                     const int pPrimaryArpeggioNoteInOctave, const int pPrimaryArpeggioOctave,
                                     const int pPrimaryPitch, const int pRatio, const int pSecondaryPeriod, const int pSecondaryArpeggioNoteInOctave, const int pSecondaryArpeggioOctave,
                                     const int pSecondaryPitch, const int pHardwareEnvelope, const bool pRetrig,
                                     const bool pIsSidActivated, const int pSidCycleBalancePercent, const int pSidLowShelfVolume, const int pSidRatio,
                                     const int pSidArpeggio, const int pSidPitch) noexcept :
        volume(pVolume),
        noise(pNoise),
        primaryPeriod(pPrimaryPeriod),
        primaryArpeggioNoteInOctave(pPrimaryArpeggioNoteInOctave),
        primaryArpeggioOctave(pPrimaryArpeggioOctave),
        primaryPitch(pPrimaryPitch),

        link(pLink),
        ratio(pRatio),
        hardwareEnvelope(pHardwareEnvelope),

        secondaryPeriod(pSecondaryPeriod),
        secondaryArpeggioNoteInOctave(pSecondaryArpeggioNoteInOctave),
        secondaryArpeggioOctave(pSecondaryArpeggioOctave),
        secondaryPitch(pSecondaryPitch),

        retrig(pRetrig),

        sidActivated(pIsSidActivated),
        sidCycleBalancePercent(pSidCycleBalancePercent),
        sidLowShelfVolume(pSidLowShelfVolume),
        sidRatio(pSidRatio),
        sidArpeggio(pSidArpeggio),
        sidPitch(pSidPitch),

        hashcode(0U)
{
    jassert(primaryArpeggioNoteInOctave < CellConstants::noteCountInOctave);
    jassert((primaryArpeggioOctave >= PsgValues::minimumArpeggioOctave) && (primaryArpeggioOctave <= PsgValues::maximumArpeggioOctave));
    jassert(secondaryArpeggioNoteInOctave < CellConstants::noteCountInOctave);
    jassert((secondaryArpeggioOctave >= PsgValues::minimumArpeggioOctave) && (secondaryArpeggioOctave <= PsgValues::maximumArpeggioOctave));
    jassert((pVolume >= PsgValues::minimumVolume) && (pVolume <= PsgValues::maximumVolumeNoHard));

    jassert((sidCycleBalancePercent >= 0) && (sidCycleBalancePercent <= 100));
    jassert((sidLowShelfVolume >= PsgValues::minimumVolume) && (sidLowShelfVolume <= PsgValues::maximumVolumeNoHard));
    jassert((sidArpeggio >= PsgValues::sidMinimumArpeggio) && (sidArpeggio <= PsgValues::sidMaximumArpeggio));
    jassert((sidPitch >= PsgValues::sidMinimumPitch) && (sidPitch <= PsgValues::sidMaximumPitch));

    calculateHashcode();
}

const PsgInstrumentCell& PsgInstrumentCell::getEmptyPsgInstrumentCell() noexcept
{
    return emptyCell;
}

PsgInstrumentCell PsgInstrumentCell::buildSoftwareCell(int volume, int noise, int arpeggioNoteInOctave, int arpeggioOctave,
        int pitch, const bool sound, int period) noexcept
{
    jassert((arpeggioNoteInOctave >= 0) && (arpeggioNoteInOctave < CellConstants::noteCountInOctave));
    jassert((arpeggioOctave >= PsgValues::minimumArpeggioOctave) && (arpeggioOctave <= PsgValues::maximumArpeggioOctave));

    return { sound ? PsgInstrumentCellLink::softOnly : PsgInstrumentCellLink::noSoftNoHard, volume, noise, period,
             arpeggioNoteInOctave, arpeggioOctave,
             pitch, PsgValues::defaultRatio, 0, 0, 0, 0,
             PsgValues::defaultHardwareEnvelope, false,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

PsgInstrumentCell PsgInstrumentCell::buildSoftToHard(int pNoise, int pArpeggioNoteInOctave, int pArpeggioOctave, int pPitch, int pRatio,
                                                     int pHardwareEnvelope, bool pRetrig) noexcept
{
    return { PsgInstrumentCellLink::softToHard, 0, pNoise, 0, pArpeggioNoteInOctave, pArpeggioOctave,
             pPitch, pRatio, 0, 0, 0,
             0, pHardwareEnvelope, pRetrig,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

PsgInstrumentCell PsgInstrumentCell::buildHardToSoft(int pNoise, int pArpeggioNoteInOctave, int pArpeggioOctave, int pPitch, int pRatio,
                                                     int pHardwareEnvelope, bool pRetrig) noexcept
{
    return { PsgInstrumentCellLink::hardToSoft, 0, pNoise, 0, pArpeggioNoteInOctave, pArpeggioOctave,
             pPitch, pRatio, 0, 0, 0,
             0, pHardwareEnvelope, pRetrig,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

PsgInstrumentCell PsgInstrumentCell::buildNoSoftwareNoHardwareCell(int pVolume, int pNoise) noexcept
{
    return { PsgInstrumentCellLink::noSoftNoHard, pVolume, pNoise, 0, 0, 0, 0,
             PsgValues::defaultRatio, 0, 0, 0, 0,
             PsgValues::defaultHardwareEnvelope, false,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

PsgInstrumentCell PsgInstrumentCell::buildHardOnly(int pNoise, int pArpeggioNoteInOctave, int pArpeggioOctave, int pPitch, int pHardwareEnvelope, bool pRetrig,
                                                   int pHardwarePeriod) noexcept
{
    return { PsgInstrumentCellLink::hardOnly, 0, pNoise, pHardwarePeriod, pArpeggioNoteInOctave, pArpeggioOctave,
             pPitch,
             PsgValues::defaultRatio, 0, 0, 0, 0,
             pHardwareEnvelope, pRetrig,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

int PsgInstrumentCell::getVolume() const noexcept
{
    return volume;
}

int PsgInstrumentCell::getNoise() const noexcept
{
    return noise;
}

PsgInstrumentCellLink PsgInstrumentCell::getLink() const noexcept
{
    return link;
}

int PsgInstrumentCell::getHardwareEnvelope() const noexcept
{
    return hardwareEnvelope;
}

bool PsgInstrumentCell::isRetrig() const noexcept
{
    return retrig;
}

bool PsgInstrumentCell::doesForcePrimaryPeriod() const noexcept
{
    return (primaryPeriod != 0);
}

int PsgInstrumentCell::getPrimaryPeriod() const noexcept
{
    return primaryPeriod;
}

int PsgInstrumentCell::getPrimaryArpeggioNoteInOctave() const noexcept
{
    return primaryArpeggioNoteInOctave;
}

int PsgInstrumentCell::getPrimaryArpeggioOctave() const noexcept
{
    return primaryArpeggioOctave;
}

int PsgInstrumentCell::getPrimaryPitch() const noexcept
{
    return primaryPitch;
}

int PsgInstrumentCell::getRatio() const noexcept
{
    return ratio;
}

bool PsgInstrumentCell::doesForceSecondaryPeriod() const noexcept
{
    return (secondaryPeriod != 0);
}

int PsgInstrumentCell::getSecondaryPeriod() const noexcept
{
    return secondaryPeriod;
}

int PsgInstrumentCell::getSecondaryArpeggioNoteInOctave() const noexcept
{
    return secondaryArpeggioNoteInOctave;
}

int PsgInstrumentCell::getSecondaryArpeggioOctave() const noexcept
{
    return secondaryArpeggioOctave;
}

int PsgInstrumentCell::getSecondaryPitch() const noexcept
{
    return secondaryPitch;
}

bool PsgInstrumentCell::isSoftware() const noexcept
{
    return (link == PsgInstrumentCellLink::noSoftNoHard) || (link == PsgInstrumentCellLink::softOnly);
}

bool PsgInstrumentCell::isHardware() const noexcept
{
    return !isSoftware();
}

bool PsgInstrumentCell::isEmpty() const noexcept
{
    return (link == PsgInstrumentCellLink::noSoftNoHard) && (volume == 0) && (noise == 0) && (primaryPeriod == 0)
        && (primaryArpeggioNoteInOctave == 0) && (primaryArpeggioOctave == 0) && (primaryPitch == 0)
        && (ratio == PsgValues::defaultRatio) && (hardwareEnvelope <= PsgValues::defaultHardwareEnvelope) && (secondaryPeriod == 0)
        && (secondaryArpeggioNoteInOctave == 0) && (secondaryArpeggioOctave == 0) && (secondaryPitch == 0) && !retrig
        && !sidActivated && (sidLowShelfVolume == PsgValues::defaultSidLowShelfVolume)
        && (sidRatio == PsgValues::defaultSidRatio) && (sidCycleBalancePercent == PsgValues::defaultSidBalancePercent)
        && (sidArpeggio == PsgValues::defaultSidArpeggio) && (sidPitch == PsgValues::defaultSidPitch)
    ;
}

bool PsgInstrumentCell::isMute() const noexcept
{
    return (volume == 0) && !sidActivated &&
            ((link == PsgInstrumentCellLink::noSoftNoHard) || (link == PsgInstrumentCellLink::softOnly));
}

bool PsgInstrumentCell::isMusicallyEqualTo(const PsgInstrumentCell& other) const noexcept // NOLINT(*-function-cognitive-complexity)
{
    // Special case first: they can be both mute (the only way the Link can be different).
    if (isMute() && other.isMute()) {
        return true;
    }

    if ((link != other.link) || (noise != other.noise)) {
        return false;
    }

    // If not hardware, the volume must match.
    if ((link == PsgInstrumentCellLink::noSoftNoHard) || (link == PsgInstrumentCellLink::softOnly)) {
        if (volume != other.volume) {
            return false;
        }
    }

    // If noSoftNoHard, nothing else to compare.
    if (link == PsgInstrumentCellLink::noSoftNoHard) {
        return true;
    }

    // The primary sounds can be compared, they are always present.
    if (primaryPeriod != other.primaryPeriod) {
        return false;
    }
    // If there is no forced period, compares the Arps and Pitch only.
    if ((primaryPeriod == 0) && ((primaryArpeggioNoteInOctave != other.primaryArpeggioNoteInOctave)
                                     || (primaryArpeggioOctave != other.primaryArpeggioOctave)
                                     || (primaryPitch != other.primaryPitch))) {
        return false;
    }

    // Checks the hardware part.
    if (isHardware()) {
        if ((hardwareEnvelope != other.hardwareEnvelope) || (retrig != other.retrig)) {
            return false;
        }

        if (link == PsgInstrumentCellLink::softToHard) {
            // In Soft To Hard, only the hardware pitch matters.
            if (secondaryPitch != other.secondaryPitch) {
                return false;
            }
        } else {
            // All other hardware sounds.
            if (secondaryPeriod != other.secondaryPeriod) {
                return false;
            }

            // If there is no forced period, compares the Arps and Pitch only.
            if ((secondaryPeriod == 0) && ((secondaryArpeggioNoteInOctave != other.secondaryArpeggioNoteInOctave)
                    || (secondaryArpeggioOctave != other.secondaryArpeggioOctave)
                    || (secondaryPitch != other.secondaryPitch))) {
                return false;
            }
        }
    }

    // Checks the software pitch for HardToSoft.
    if ((link == PsgInstrumentCellLink::hardToSoft) && (primaryPitch != other.primaryPitch)) {
        return false;
    }
    // Checks the hardware pitch for SoftToHard.
    if ((link == PsgInstrumentCellLink::softToHard) && (secondaryPitch != other.secondaryPitch)) {
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

bool PsgInstrumentCell::operator<(const PsgInstrumentCell& other) const noexcept
{
    return (hashcode < other.hashcode);
}

bool PsgInstrumentCell::operator==(const PsgInstrumentCell& other) const noexcept
{
    return (hashcode == other.hashcode);
}

bool PsgInstrumentCell::operator!=(const PsgInstrumentCell& other) const noexcept
{
    return (hashcode != other.hashcode);
}

bool PsgInstrumentCell::isRatioUseful() const noexcept
{
    return ((link == PsgInstrumentCellLink::softToHard) || (link == PsgInstrumentCellLink::hardToSoft));
}

bool PsgInstrumentCell::isSidActivated() const noexcept
{
    return sidActivated;
}

int PsgInstrumentCell::getSidBalancePercent() const noexcept
{
    return sidCycleBalancePercent;
}

int PsgInstrumentCell::getSidLowShelfVolume() const noexcept
{
    return sidLowShelfVolume;
}

int PsgInstrumentCell::getSidRatio() const noexcept
{
    return sidRatio;
}

int PsgInstrumentCell::getSidArpeggio() const noexcept
{
    return sidArpeggio;
}

int PsgInstrumentCell::getSidPitch() const noexcept
{
    return sidPitch;
}

size_t PsgInstrumentCell::getHashcode() const noexcept
{
    return hashcode;
}

PsgInstrumentCell PsgInstrumentCell::with(const OptionalValue<PsgInstrumentCellLink> newLink, const OptionalInt newVolume, const OptionalInt newNoise, const OptionalInt newPrimaryPeriod,
                                          const OptionalInt newPrimaryArpeggioNoteInOctave, const OptionalInt newPrimaryArpeggioOctave,
                                          const OptionalInt newPrimaryPitch, const OptionalInt newRatio, const OptionalInt newSecondaryPeriod,
                                          const OptionalInt newSecondaryArpeggioNoteInOctave, const OptionalInt newSecondaryArpeggioOctave,
                                          const OptionalInt newSecondaryPitch, const OptionalInt newHardwareEnvelope,
                                          const OptionalBool newRetrig,
                                          const OptionalBool newIsSidActivated, const OptionalInt newSidCycleBalancePercent, const OptionalInt newSidLowShelfVolume,
                                          const OptionalInt newSidRatio, const OptionalInt newSidArpeggio, const OptionalInt newSidPitch
                                        ) const noexcept
{
    return {
            newLink.isPresent() ? newLink.getValue() : link,
            newVolume.isPresent() ? newVolume.getValue() : volume,
            newNoise.isPresent() ? newNoise.getValue() : noise,
            newPrimaryPeriod.isPresent() ? newPrimaryPeriod.getValue() : primaryPeriod,
            newPrimaryArpeggioNoteInOctave.isPresent() ? newPrimaryArpeggioNoteInOctave.getValue() : primaryArpeggioNoteInOctave,
            newPrimaryArpeggioOctave.isPresent() ? newPrimaryArpeggioOctave.getValue() : primaryArpeggioOctave,
            newPrimaryPitch.isPresent() ? newPrimaryPitch.getValue() : primaryPitch,
            newRatio.isPresent() ? newRatio.getValue() : ratio,
            newSecondaryPeriod.isPresent() ? newSecondaryPeriod.getValue() : secondaryPeriod,
            newSecondaryArpeggioNoteInOctave.isPresent() ? newSecondaryArpeggioNoteInOctave.getValue() : secondaryArpeggioNoteInOctave,
            newSecondaryArpeggioOctave.isPresent() ? newSecondaryArpeggioOctave.getValue() : secondaryArpeggioOctave,
            newSecondaryPitch.isPresent() ? newSecondaryPitch.getValue() : secondaryPitch,
            newHardwareEnvelope.isPresent() ? newHardwareEnvelope.getValue() : hardwareEnvelope,
            newRetrig.isPresent() ? newRetrig.getValue() : retrig,
            newIsSidActivated.isPresent() ? newIsSidActivated.getValue() : sidActivated,
            newSidCycleBalancePercent.isPresent() ? newSidCycleBalancePercent.getValue() : sidCycleBalancePercent,
            newSidLowShelfVolume.isPresent() ? newSidLowShelfVolume.getValue() : sidLowShelfVolume,
            newSidRatio.isPresent() ? newSidRatio.getValue() : sidRatio,
            newSidArpeggio.isPresent() ? newSidArpeggio.getValue() : sidArpeggio,
            newSidPitch.isPresent() ? newSidPitch.getValue() : sidPitch,
    };
}

PsgInstrumentCell PsgInstrumentCell::withLink(const PsgInstrumentCellLink newLink) const noexcept
{
    return with(newLink);
}

PsgInstrumentCell PsgInstrumentCell::withNoise(const int newNoise) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), newNoise);
}

PsgInstrumentCell PsgInstrumentCell::withVolume(const int newVolume) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), newVolume);
}

PsgInstrumentCell PsgInstrumentCell::withEnvelopeAndRatio(const int newEnvelope, const int newRatio) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                newRatio, OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                newEnvelope);
}

PsgInstrumentCell PsgInstrumentCell::withEnvelope(const int newEnvelope) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                newEnvelope);
}

PsgInstrumentCell PsgInstrumentCell::withRatio(const int newRatio) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                newRatio);
}

PsgInstrumentCell PsgInstrumentCell::withRetrig(const bool newRetrig) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
         OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
         OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
         OptionalInt(), newRetrig);
}

PsgInstrumentCell PsgInstrumentCell::withPrimaryPeriod(const int newPrimaryPeriod) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(), newPrimaryPeriod);
}

PsgInstrumentCell PsgInstrumentCell::withPrimaryArpeggioNoteInOctave(const int newPrimaryArpeggioNoteInOctave) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(), OptionalInt(),
            newPrimaryArpeggioNoteInOctave);
}

PsgInstrumentCell PsgInstrumentCell::withPrimaryArpeggioOctave(const int newPrimaryArpeggioOctave) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), newPrimaryArpeggioOctave);
}

PsgInstrumentCell PsgInstrumentCell::withPrimaryPitch(const int newPrimaryPitch) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), OptionalInt(),
            newPrimaryPitch);
}

PsgInstrumentCell PsgInstrumentCell::withSecondaryPeriod(const int newSecondaryPeriod) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), newSecondaryPeriod);
}

PsgInstrumentCell PsgInstrumentCell::withSecondaryArpeggioNoteInOctave(const int newSecondaryArpeggioNoteInOctave) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(), newSecondaryArpeggioNoteInOctave);
}

PsgInstrumentCell PsgInstrumentCell::withSecondaryArpeggioOctave(const int newSecondaryArpeggioOctave) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(), OptionalInt(),
                newSecondaryArpeggioOctave);
}

PsgInstrumentCell PsgInstrumentCell::withSecondaryPitch(const int newSecondaryPitch) const noexcept
{
    return with(OptionalValue<PsgInstrumentCellLink>(), OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), newSecondaryPitch);
}

PsgInstrumentCell PsgInstrumentCell::withSid(const bool newIsSidActivated, const int newSidCycleBalancePercent, const int newSidLowShelfVolume, const int newSidRatio,
    const int newSidArpeggio, const int newSidPitch) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), OptionalInt(),
                OptionalInt(), OptionalInt(), {},
                {}, {},
                newIsSidActivated, newSidCycleBalancePercent, newSidLowShelfVolume, newSidRatio,
                newSidArpeggio, newSidPitch
    );
}

PsgInstrumentCell PsgInstrumentCell::withSidActivated(const bool newIsSidActivated) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), {},
            {}, {},
            newIsSidActivated
    );
}

PsgInstrumentCell PsgInstrumentCell::withSidRatio(const int newSidRatio) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), OptionalInt(),
            OptionalInt(), OptionalInt(), {},
            {}, {},
            {}, {}, {}, newSidRatio
    );
}

PsgInstrumentCell PsgInstrumentCell::withSidBalancePercent(const int newBalancePercent) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), {},
        {}, {},
        {}, newBalancePercent
);
}

PsgInstrumentCell PsgInstrumentCell::withSidLowShelfVolume(const int newLowShelfVolume) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), {},
        {}, {},
        {}, {}, newLowShelfVolume
    );
}

PsgInstrumentCell PsgInstrumentCell::withSidArpeggio(const int newArpeggio) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), {},
        {}, {},
        {}, {}, {}, {}, newArpeggio
    );
}

PsgInstrumentCell PsgInstrumentCell::withSidPitch(const int newPitch) const noexcept
{
    return with({}, OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), OptionalInt(),
        OptionalInt(), OptionalInt(), {},
        {}, {},
        {}, {}, {}, {}, {}, newPitch
    );
}

void PsgInstrumentCell::calculateHashcode() noexcept
{
    size_t result = 17U;

    result = (31U * result) + (static_cast<unsigned int>(volume) & 0xfU);
    result = (31U * result) + (static_cast<unsigned int>(noise) & 0x1fU);
    result = (31U * result) + (static_cast<unsigned int>(primaryPeriod) & 0xfffU);
    result = (31U * result) + (static_cast<unsigned int>(primaryArpeggioNoteInOctave) & 0xffU);
    result = (31U * result) + (static_cast<unsigned int>(primaryArpeggioOctave) & 0xffU);
    result = (31U * result) + (static_cast<unsigned int>(primaryPitch) & 0xffffU);

    result = (31U * result) + static_cast<size_t>(link);
    result = (31U * result) + (static_cast<unsigned int>(ratio) & 7U);

    result = (31U * result) + (static_cast<unsigned int>(hardwareEnvelope) & 0xfU);
    result = (31U * result) + (static_cast<unsigned int>(secondaryPeriod) & 0xffffU);
    result = (31U * result) + (static_cast<unsigned int>(secondaryArpeggioNoteInOctave) & 0xffU);
    result = (31U * result) + (static_cast<unsigned int>(secondaryArpeggioOctave) & 0xffU);
    result = (31U * result) + (static_cast<unsigned int>(secondaryPitch) & 0xffffU);

    result = (31U * result) + (retrig ? 1U : 0U);

    result = (31U * result) + (sidActivated ? 1U : 0U);
    result = (31U * result) + static_cast<unsigned int>(sidCycleBalancePercent);
    result = (31U * result) + static_cast<unsigned int>(sidLowShelfVolume);
    result = (31U * result) + static_cast<unsigned int>(sidRatio);
    result = (31U * result) + static_cast<unsigned int>(sidArpeggio);
    result = (31U * result) + static_cast<unsigned int>(sidPitch);

    hashcode = result;
}

}   // namespace arkostracker

