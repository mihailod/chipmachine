#pragma once

#include "PsgInstrumentCellLink.h"

namespace arkostracker 
{

/**
 * Represents a "flat" PSG cell, which can be directly used by a player (C++ or Z80).
 * It used to be a PsgInstrumentCell (in AT2).
 *
 * They are generated from a PsgInstrumentCell, but only the PsgPart can do that, because it needs to know all the Cells
 * to generate one (because of the auto-spread feature).
 * A LowLevelPsgInstrumentCell is ideal for a player, but should NOT be serialized, as they wouldn't contain all the data.
 */
class LowLevelPsgInstrumentCell               // TODO TU this.
{
public:
    /** Constructor of an empty cell (no sound). */
    LowLevelPsgInstrumentCell() noexcept;

    /**
     * Constructor for a full-blown PSG Instrument.
     * @param link the link.
     * @param volume the volume (0 to 15).
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param softwarePeriod 0 = auto, else the period.
     * @param softwareArpeggio the software arpeggio.
     * @param softwarePitch the software pitch.
     * @param ratio the ratio.
     * @param hardwareEnvelope the envelope. Possibly from 0-15, but will be modified and stored as 8-15, AT3 only understand those.
     * @param hardwarePeriod 0 = auto, else the period.
     * @param hardwareArpeggio the hardware arpeggio
     * @param hardwarePitch the hardware pitch.
     * @param retrig true if retrig.
     */
    LowLevelPsgInstrumentCell(PsgInstrumentCellLink link, int volume, int noise,
        int softwarePeriod, int softwareArpeggio, int softwarePitch,
        int ratio, int hardwareEnvelope,
        int hardwarePeriod, int hardwareArpeggio, int hardwarePitch, bool retrig,
        bool isSidActivated, int sidCycleBalancePercent, int sidLowShelfVolume, int sidRatio, int sidArpeggio, int sidPitch) noexcept;

    /** Returns a share instance of an empty PSG Instrument Cell. As it is immutable, it can be shared without any problem. */
    static const LowLevelPsgInstrumentCell& getEmptyCell() noexcept;

    /**
     * Static constructor for building a simple software cell.
     * @param volume the volume (0 to 15).
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param arpeggio the arpeggio.
     * @param pitch the software pitch.
     * @param sound true to open the sound channel.
     */
    static LowLevelPsgInstrumentCell buildSoftwareCell(int volume, int noise = 0, int arpeggio = 0, int pitch = 0, bool sound = true) noexcept;

    /** Returns the volume, from 0 to 15. */
    int getVolume() const noexcept;
    /** Returns the noise, from 0 to 31. 0 = no noise. */
    int getNoise() const noexcept;
    /** Returns the link between software and hardware */
    PsgInstrumentCellLink getLink() const noexcept;

    /** Returns the hardware envelope (8-15). */
    int getHardwareEnvelope() const noexcept;
    /** True if the hardware curve must be retriggered. */
    bool isRetrig() const noexcept;

    /** Indicates whether the software period is forced, or if automatic. */
    bool doesForceSoftwarePeriod() const noexcept;
    /** Returns the software period (0 = automatic). */
    int getSoftwarePeriod() const noexcept;
    /** Returns the possible software arpeggio (may be 0). */
    int getSoftwareArpeggio() const noexcept;
    /** Returns the possible software pitch (may be 0).*/
    int getSoftwarePitch() const noexcept;

    /** Returns the ratio between the software and hardware period, or the opposite (0-7). */
    int getRatio() const noexcept;

    /** Indicates whether the hardware period is forced, or if automatic. */
    bool doesForceHardwarePeriod() const noexcept;
    /** Returns the hardware period (0 = automatic). */
    int getHardwarePeriod() const noexcept;
    /** Returns the possible hardware arpeggio (may be 0). */
    int getHardwareArpeggio() const noexcept;
    /** Returns the possible hardware pitch (may be 0).*/
    int getHardwarePitch() const noexcept;

    /** @return true if hardware sound is used. */
    bool isHardware() const noexcept;
    /** @return true if the Cell is empty (no sound at all, only default value, no relevant ones). */
    bool isEmpty() const noexcept;

    /** @return true if SID is activated. */
    bool isSidActivated() const noexcept;
    /** @return the percent of the SID balance (high/low shelf). */
    int getSidBalancePercent() const noexcept;
    /** @return the volume of the SID low-shelf volume (from 0-15). */
    int getSidLowShelfVolume() const noexcept;
    /** @return the SID ratio from 0 to 4. */
    int getSidRatio() const noexcept;
    /** @return the SID arpeggio, from 0. */
    int getSidArpeggio() const noexcept;
    /** @return the SID pitch. */
    int getSidPitch() const noexcept;

    /** Comparison operator, used for sets. */
    bool operator<(const LowLevelPsgInstrumentCell& other) const noexcept;
    bool operator==(const LowLevelPsgInstrumentCell& other) const noexcept;
    bool operator!=(const LowLevelPsgInstrumentCell& other) const noexcept;

    /**
     * Compares with another Cell to check the equality. The comparison is "musical", in the sense that
     * what is compared is done according to the Link. For example, no need to compare the hardware values if the Link is SoftwareOnly.
     * @param other the other Cell.
     * @return true if musically equal.
     */
    bool equalMusically(const LowLevelPsgInstrumentCell& other) const noexcept;

    /** Returns the (cached) hashcode. */
    size_t getHashcode() const noexcept;

private:
    static const LowLevelPsgInstrumentCell emptyCell;             // Shared instance of an empty Cell.

    /** Calculates the hashcode and stores it. */
    void calculateHashcode() noexcept;

    int volume;                      // A volume, from 0 to 15.
    int noise;                       // The noise, from 0 to 31. 0 = no noise.
    int softwarePeriod;              // The software period. 0 = automatic.
    int softwareArpeggio;            // The software arpeggio. Useful only if the software period is automatic.
    int softwarePitch;               // The software pitch.

    PsgInstrumentCellLink link;      // The link between software and hardware.
    int ratio;                       // The ratio, from 0 to 7.

    int hardwareEnvelope;            // The hardware envelope, from 8 to 15.
    int hardwarePeriod;              // The hardware period. 0 = automatic.
    int hardwareArpeggio;            // The hardware arpeggio. Useful only if the hardware period is automatic.
    int hardwarePitch;               // The hardware pitch.

    bool retrig;                     // True to retrig the hardware curve. Only relevant in case of an hardware sound.

    bool sidActivated;
    int sidCycleBalancePercent;             // From 0 to 100.
    int sidLowShelfVolume;                  // From 0 to 15.
    int sidRatio;                           // From 0 to 4 (2 is "normal").
    int sidArpeggio;                        // From 0.
    int sidPitch;

    size_t hashcode;                 // The hashcode, calculated once and for all.
};

}   // namespace arkostracker
