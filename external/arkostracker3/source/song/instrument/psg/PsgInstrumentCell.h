#pragma once

#include "../../../utils/OptionalValue.h"
#include "PsgInstrumentCellLink.h"

namespace arkostracker 
{

/**
 * A Cell of a PSG Instrument. It is immutable. It is different of a cell from AT2,
 * as it has a primary and secondary arp/pitch/forced period.
 * Note that the stored values can go beyond the limits. For example, in HardOnly, the primary period should be
 * between 0 and 0xffff. If switching to SoftOnly, it will still be 0xffff. But when generating a Flag cell,
 * the limit will be applied.
 */
class PsgInstrumentCell             // TODO TU this.
{
public:
    /** Constructor of an empty cell (no sound). */
    PsgInstrumentCell() noexcept;

     /**
     * Constructor for a full-blown PSG Instrument.
      * @param link the link.
      * @param volume the volume (0 to 15).
      * @param noise the noise, from 0 to 31. 0 = no noise.
      * @param primaryPeriod 0 = auto, else the period.
      * @param primaryArpeggioNoteInOctave the primary arpeggio, in an octave (0-11).
      * @param primaryArpeggioOctave the primary arpeggio octave (may be negative).
      * @param primaryPitch the primary pitch.
      * @param ratio the ratio (0-7).
      * @param secondaryPeriod 0 = auto, else the period.
      * @param secondaryArpeggioNoteInOctave the secondary arpeggio, in an octave (0-11).
      * @param secondaryArpeggioOctave the secondary arpeggio octave (may be negative).
      * @param secondaryPitch the secondary pitch.
      * @param hardwareEnvelope the envelope. Possibly from 0-15, but will be modified and stored as 8-15, AT3 only understand those.
      * @param retrig true if retrig.
      * @param isSidActivated true if SID is activated.
      * @param sidCycleBalancePercent the balance, from 0 to 100 (50 is default).
      * @param sidLowShelfVolume the low shelf volume (0 by default).
      * @param sidRatio the SID ratio (2 is default).
      * @param sidArpeggio the SID arpeggio (>=0).
      * @param sidPitch the SID pitch (may be negative).
      */
    PsgInstrumentCell(PsgInstrumentCellLink link, int volume, int noise,
                      int primaryPeriod, int primaryArpeggioNoteInOctave, int primaryArpeggioOctave, int primaryPitch,
                      int ratio,
                      int secondaryPeriod, int secondaryArpeggioNoteInOctave, int secondaryArpeggioOctave, int secondaryPitch,
                      int hardwareEnvelope, bool retrig,
                      bool isSidActivated, int sidCycleBalancePercent, int sidLowShelfVolume, int sidRatio, int sidArpeggio, int sidPitch
    ) noexcept;

    /** Returns a share instance of an empty PSG Instrument Cell. As it is immutable, it can be shared without any problem. */
    static const PsgInstrumentCell& getEmptyPsgInstrumentCell() noexcept;

    /**
     * Static constructor for building a simple software cell.
     * @param volume the volume (0 to 15).
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param arpeggioNoteInOctave the arpeggio note in the octave (0-11).
     * @param arpeggioOctave the arpeggio octave (may be negative).
     * @param pitch the primary pitch.
     * @param sound true to open the sound channel.
     * @param period the period (0=auto).
     * @return the cell.
     */
    static PsgInstrumentCell buildSoftwareCell(int volume, int noise = 0, int arpeggioNoteInOctave = 0, int arpeggioOctave = 0,
            int pitch = 0, bool sound = true, int period = 0) noexcept;

    /**
     * Static constructor for building a no soft/no hard cell.
     * @param volume the volume (0 to 15).
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @return the cell.
     */
    static PsgInstrumentCell buildNoSoftwareNoHardwareCell(int volume = 0, int noise = 0) noexcept;

    /**
     * Static constructor for building a soft to hard cell.
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param arpeggioNoteInOctave the arpeggio note in the octave (0-11).
     * @param arpeggioOctave the arpeggio octave (may be negative).
     * @param pitch the software pitch.
     * @param ratio the ratio (0-7).
     * @param hardwareEnvelope the hardware envelope (8-15).
     * @param retrig true to retrig.
     * @return the cell.
     */
    static PsgInstrumentCell buildSoftToHard(int noise = 0, int arpeggioNoteInOctave = 0, int arpeggioOctave = 0,
            int pitch = 0, int ratio = 4, int hardwareEnvelope = 8, bool retrig = false) noexcept;

    /**
     * Static constructor for building a Hard To Soft cell.
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param arpeggioNoteInOctave the arpeggio note in the octave (0-11).
     * @param arpeggioOctave the arpeggio octave (may be negative).
     * @param pitch the software pitch.
     * @param ratio the ratio (0-7).
     * @param hardwareEnvelope the hardware envelope (8-15).
     * @param retrig true to retrig.
     * @return the cell.
     */
    static PsgInstrumentCell buildHardToSoft(int noise = 0, int arpeggioNoteInOctave = 0, int arpeggioOctave = 0,
                                             int pitch = 0, int ratio = 4, int hardwareEnvelope = 8, bool retrig = false) noexcept;

    /**
     * Static constructor for building a Hard Only cell.
     * @param noise the noise, from 0 to 31. 0 = no noise.
     * @param arpeggioNoteInOctave the arpeggio note in the octave (0-11).
     * @param arpeggioOctave the arpeggio octave (may be negative).
     * @param pitch the software pitch.
     * @param hardwareEnvelope the hardware envelope (8-15).
     * @param retrig true to retrig.
     * @param hardwarePeriod the forced hardware period, or 0 for auto.
     * @return the cell.
     */
    static PsgInstrumentCell buildHardOnly(int noise = 0, int arpeggioNoteInOctave = 0, int arpeggioOctave = 0,
            int pitch = 0, int hardwareEnvelope = 8, bool retrig = false, int hardwarePeriod = 0) noexcept;

    /** @return the volume, from 0 to 15. */
    int getVolume() const noexcept;
    /** @return the noise, from 0 to 31. 0 = no noise. */
    int getNoise() const noexcept;
    /** @return the link between primary and secondary */
    PsgInstrumentCellLink getLink() const noexcept;

    /** @return the hardware envelope (8-15). */
    int getHardwareEnvelope() const noexcept;
    /** True if the hardware curve must be retriggered. */
    bool isRetrig() const noexcept;

    /** Indicates whether the primary period is forced, or if automatic. */
    bool doesForcePrimaryPeriod() const noexcept;
    /** @return the primary period (0 = automatic). */
    int getPrimaryPeriod() const noexcept;
    /** @return the possible primary arpeggio note in an octave (from 0 to 11). */
    int getPrimaryArpeggioNoteInOctave() const noexcept;
    /** @return the possible primary arpeggio octave (may be negative). */
    int getPrimaryArpeggioOctave() const noexcept;
    /** @return the possible primary pitch (may be 0).*/
    int getPrimaryPitch() const noexcept;

    /** @return the ratio between the primary and secondary period, or the opposite (0-7). */
    int getRatio() const noexcept;

    /** Indicates whether the secondary period is forced, or if automatic. */
    bool doesForceSecondaryPeriod() const noexcept;
    /** @return the secondary period (0 = automatic). */
    int getSecondaryPeriod() const noexcept;
    /** @return the possible secondary arpeggio note in an octave (from 0 to 11). */
    int getSecondaryArpeggioNoteInOctave() const noexcept;
    /** @return the possible secondary arpeggio octave (may be negative). */
    int getSecondaryArpeggioOctave() const noexcept;
    /** @return the possible secondary pitch (may be 0).*/
    int getSecondaryPitch() const noexcept;

    /** @return true if software sound is used (noSoftNoHard, softwareOnly). */
    bool isSoftware() const noexcept;
    /** @return true if hardware sound is used. */
    bool isHardware() const noexcept;
    /** @return true if the Cell is empty (no sound at all, only default value, no relevant ones). */
    bool isEmpty() const noexcept;
    /** @return true if the Cell is does not emit sound (no soft no hard with volume 0, software with volume 0). */
    bool isMute() const noexcept;

    /**
     * @return true if "musicality" is equal, in the sense that what is compared is done according to the Link.
     * For example, no need to compare the hardware values if the Link is SoftwareOnly.
     * @param other the other Cell.
     */
    bool isMusicallyEqualTo(const PsgInstrumentCell& other) const noexcept;

    /** @return true if the ratio is useful: only for soft to hard and hard to soft. */
    bool isRatioUseful() const noexcept;

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
    bool operator<(const PsgInstrumentCell& other) const noexcept;
    bool operator==(const PsgInstrumentCell& other) const noexcept;
    bool operator!=(const PsgInstrumentCell& other) const noexcept;

    /** @return the (cached) hashcode. */
    size_t getHashcode() const noexcept;

    /** @return a copy of the Cell, with new parameters. Only changed ones must be set. */
    PsgInstrumentCell with(OptionalValue<PsgInstrumentCellLink> newLink = OptionalValue<PsgInstrumentCellLink>(),
            OptionalInt newVolume = OptionalInt(), OptionalInt newNoise = OptionalInt(),
            OptionalInt newPrimaryPeriod = OptionalInt(), OptionalInt newPrimaryArpeggioNoteInOctave = OptionalInt(),
            OptionalInt newPrimaryArpeggioOctave = OptionalInt(), OptionalInt newPrimaryPitch = OptionalInt(),
            OptionalInt newRatio = OptionalInt(),
            OptionalInt newSecondaryPeriod = OptionalInt(),
            OptionalInt newSecondaryArpeggioNoteInOctave = OptionalInt(), OptionalInt newSecondaryArpeggioOctave = OptionalInt(),
            OptionalInt newSecondaryPitch = OptionalInt(),
            OptionalInt newHardwareEnvelope = OptionalInt(), OptionalBool newRetrig = OptionalBool(),
            OptionalBool newIsSidActivated = {}, OptionalInt newSidCycleBalancePercent = {}, OptionalInt newSidLowShelfVolume = {}, OptionalInt newSidRatio = {},
            OptionalInt newSidArpeggio = {}, OptionalInt newSidPitch = {}
    ) const noexcept;

    /** @return a copy of the Cell, with a new Link. */
    PsgInstrumentCell withLink(PsgInstrumentCellLink newLink) const noexcept;
    /** @return a copy of the Cell, with a new Noise. */
    PsgInstrumentCell withNoise(int newNoise) const noexcept;
    /** @return a copy of the Cell, with a new volume. */
    PsgInstrumentCell withVolume(int newVolume) const noexcept;
    /** @return a copy of the Cell, with a new envelope (0-15) and ratio (0-7). */
    PsgInstrumentCell withEnvelopeAndRatio(int newEnvelope, int newRatio) const noexcept;
    /** @return a copy of the Cell, with a new envelope (0-15). */
    PsgInstrumentCell withEnvelope(int newEnvelope) const noexcept;
    /** @return a copy of the Cell, with a new ratio (0-7). */
    PsgInstrumentCell withRatio(int newRatio) const noexcept;
    /** @return a copy of the Cell, with a new retrig. */
    PsgInstrumentCell withRetrig(bool newRetrig) const noexcept;

    /** @return a copy of the Cell, with a new primary period. */
    PsgInstrumentCell withPrimaryPeriod(int newPrimaryPeriod) const noexcept;
    /** @return a copy of the Cell, with a new primary arpeggio note in octave. */
    PsgInstrumentCell withPrimaryArpeggioNoteInOctave(int newPrimaryArpeggioNoteInOctave) const noexcept;
    /** @return a copy of the Cell, with a new primary arpeggio octave. */
    PsgInstrumentCell withPrimaryArpeggioOctave(int newPrimaryArpeggioOctave) const noexcept;
    /** @return a copy of the Cell, with a new primary pitch. */
    PsgInstrumentCell withPrimaryPitch(int newPrimaryPitch) const noexcept;

    /** @return a copy of the Cell, with a new secondary period. */
    PsgInstrumentCell withSecondaryPeriod(int newSecondaryPeriod) const noexcept;
    /** @return a copy of the Cell, with a new secondary arpeggio note in octave. */
    PsgInstrumentCell withSecondaryArpeggioNoteInOctave(int newSecondaryArpeggioNoteInOctave) const noexcept;
    /** @return a copy of the Cell, with a new secondary arpeggio octave. */
    PsgInstrumentCell withSecondaryArpeggioOctave(int newSecondaryArpeggioOctave) const noexcept;
    /** @return a copy of the Cell, with a new secondary pitch. */
    PsgInstrumentCell withSecondaryPitch(int newSecondaryPitch) const noexcept;
    
    /** @return a copy of the Cell, with new SID data. */
    PsgInstrumentCell withSid(bool isSidActivated, int sidCycleBalancePercent, int sidLowShelfVolume, int sidRatio, int sidArpeggio, int newSidPitch) const noexcept;
    /** @return a copy of the Cell, with a new SID Activated. */
    PsgInstrumentCell withSidActivated(bool isSidActivated) const noexcept;
    /** @return a copy of the Cell, with a new SID Ratio. */
    PsgInstrumentCell withSidRatio(int ratio) const noexcept;
    /** @return a copy of the Cell, with a new SID balance percent. */
    PsgInstrumentCell withSidBalancePercent(int newBalancePercent) const noexcept;
    /** @return a copy of the Cell, with a new SID low-shelf volume. */
    PsgInstrumentCell withSidLowShelfVolume(int newLowShelfVolume) const noexcept;
    /** @return a copy of the Cell, with a new SID arpeggio. */
    PsgInstrumentCell withSidArpeggio(int newArpeggio) const noexcept;
    /** @return a copy of the Cell, with a new SID pitch. */
    PsgInstrumentCell withSidPitch(int newPitch) const noexcept;

private:
    static const PsgInstrumentCell emptyCell;      // Shared instance of an empty PsgInstrumentCell.

    /** Calculates the hashcode and stores it. */
    void calculateHashcode() noexcept;

    int volume;                             // A volume, from 0 to 15.
    int noise;                              // The noise, from 0 to 31. 0 = no noise.
    int primaryPeriod;                      // The primary period. 0 = automatic.
    int primaryArpeggioNoteInOctave;        // The primary arpeggio note in an octave. Useful only if the primary period is automatic.
    int primaryArpeggioOctave;              // The primary arpeggio octave. Useful only if the primary period is automatic.
    int primaryPitch;                       // The primary pitch.

    PsgInstrumentCellLink link;             // The link between primary and secondary envelope.
    int ratio;                              // The ratio, from 0 to 7.

    int hardwareEnvelope;                   // The hardware envelope, from 8 to 15.
    int secondaryPeriod;                    // The secondary period. 0 = automatic.
    int secondaryArpeggioNoteInOctave;      // The secondary arpeggio note in an octave. Useful only if the secondary period is automatic.
    int secondaryArpeggioOctave;            // The secondary arpeggio octave. Useful only if the secondary period is automatic.
    int secondaryPitch;                     // The secondary pitch.

    bool retrig;                            // True to retrig the hardware curve. Only relevant in case of an hardware sound.

    bool sidActivated;
    int sidCycleBalancePercent;             // From 0 to 100.
    int sidLowShelfVolume;                  // From 0 to 15.
    int sidRatio;                           // From 0 to 4 (2 is "normal").
    int sidArpeggio;                        // From 0.
    int sidPitch;

    size_t hashcode;                        // The hashcode, calculated once and for all.
};



}   // namespace arkostracker

