#pragma once

#include "../../../../song/instrument/psg/PsgInstrumentCellLink.h"
#include "../../../editorWithBars/view/component/BarAndCaptionData.h"
#include "../../../editorWithBars/view/component/BarAreaSize.h"
#include "../../../editorWithBars/view/component/BarCaptionDisplayedData.h"

namespace arkostracker 
{

class PsgInstrumentCell;

/** Helper that creates the data for each Bar Area. */
class BarDataCreator                // TODO TU this!
{
public:

    /**
     * Creates a BarData for the Link.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createLinkBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;

    /**
     * Creates a BarData for the Envelope.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createEnvelopeBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
            bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Noise.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createNoiseBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;


    // ----------------------------------------------------

    /**
     * Creates a BarData for the Primary Period.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPrimaryPeriodBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
            bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Primary Arpeggio note in octave.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPrimaryArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered,
            bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Primary Arpeggio Octave.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPrimaryArpeggioOctaveBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
            bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Primary Pitch.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPrimaryPitchBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                       bool hovered, bool selected, bool withCursor) noexcept;

    // ----------------------------------------------------

    /**
     * Creates a BarData for the Secondary Period.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSecondaryPeriodBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                        bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Secondary Arpeggio note in octave.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSecondaryArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered,
                                                                      bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Secondary Arpeggio Octave.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSecondaryArpeggioOctaveBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                                bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the Secondary Pitch.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSecondaryPitchBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                       bool hovered, bool selected, bool withCursor) noexcept;


    // ----------------------------------------------------

    /**
     * Creates a BarData for the SID activated and ratio.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSidIsActivatedAndRatioBarData(const PsgInstrumentCell& cell, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                       bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Creates a BarData for the SID balance percent.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSidBalancePercentBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;

    /**
     * Creates a BarData for the SID low-shelf volume.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSidLowShelfVolumeBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;

    /**
     * Creates a BarData for the SID arpeggio.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSidArpeggioBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;

    /**
     * Creates a BarData for the SID pitch.
     * @param cell the Cell to get the data from.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createSidPitchBarData(const PsgInstrumentCell& cell, bool withinLoop, bool outOfBounds, bool generated, bool hovered, bool selected,
        bool withCursor) noexcept;

    // ----------------------------------------------------

    /**
     * Generic method to create a BarData for the the primary/secondary Period.
     * @param cell the Cell to get the data from.
     * @param areaSize the current size of the area.
     * @param primary true if primary, false if secondary.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPeriodBarData(const PsgInstrumentCell& cell, bool primary, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
            bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * Generic method to create a BarData for the primary/secondary Arpeggio Note in Octave.
     * @param cell the Cell to get the data from.
     * @param primary true if primary, false if secondary.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createArpeggioNoteInOctaveBarData(const PsgInstrumentCell& cell, bool primary, bool withinLoop, bool outOfBounds, bool generated, bool hovered,
            bool selected, bool withCursor) noexcept;

    /**
     * Generic method to create a BarData for the primary/secondary Arpeggio Octave.
     * @param cell the Cell to get the data from.
     * @param primary true if primary, false if secondary.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createArpeggioOctaveBarData(const PsgInstrumentCell& cell, bool primary, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
                                                                bool hovered, bool selected, bool withCursor) noexcept;

    /**
     * @return true if the arpeggio or the pitch is ignored.
     * @param cell the Cell to get the data from.
     * @param primary true if primary, false if secondary.
     * @param arpeggio true if arpeggio, false if pitch.
     */
    static bool checkIfArpeggioOrPitchIgnored(const PsgInstrumentCell& cell, bool primary, bool arpeggio) noexcept;

    /**
     * Generic method to create a BarData for the primary/secondary Pitch.
     * @param cell the Cell to get the data from.
     * @param primary true if primary, false if secondary.
     * @param areaSize the current size of the area.
     * @param withinLoop true if withing the loop.
     * @param outOfBounds true if out of bounds (after end).
     * @param generated true if generated (spread).
     * @param hovered true if hovered by the mouse.
     * @param selected true if the bar is selected.
     * @param withCursor true if the cursor is here.
     * @return the BarData.
     */
    static BarAndCaptionData createPitchBarData(const PsgInstrumentCell& cell, bool primary, BarAreaSize areaSize, bool withinLoop, bool outOfBounds, bool generated,
            bool hovered, bool selected, bool withCursor) noexcept;

    /** @return the min/max values for the sound type. */
    static std::pair<int, int> getSoundTypeMinMax() noexcept;
    /** @return the smallest size for the given value for the sound type.*/
    static BarAreaSize getSizeForSoundType(PsgInstrumentCellLink link) noexcept;

    /**
     * @return the min/max values for the envelope.
     * @param barAreaSize the area size.
     * @param isHardware true if the cell is hardware.
     */
    static std::pair<int, int> getEnvelopeMinMax(BarAreaSize barAreaSize, bool isHardware) noexcept;
    /**
     * @return the minimum size that contains the given envelope.
     * @param isHardware true if the sound is hardware.
     * @param hardwareEnvelope the hardware envelope, from 8-15.
     * @param forceShrinkFull true to shrink to full, where "normal" could be shown for more clarity.
     */
    static BarAreaSize getSizeForEnvelope(bool isHardware, int hardwareEnvelope, bool forceShrinkFull);

    /** @return the min/max values for the noise. */
    static std::pair<int, int> getNoiseMinMax() noexcept;
    /** @return the minimum size that contains the given noise. */
    static BarAreaSize getSizeForNoise(int noise, bool forceShrinkFull) noexcept;

    /** @return the min/max values for a percent. */
    static std::pair<int, int> getPercentMinMax() noexcept;

    /** @return the min/max values for the period and the given bar area size. */
    static std::pair<int, int> getPeriodMinMax(BarAreaSize barAreaSize) noexcept;
    /** @return a map linking the min/max values for all the area size, for periods. All the sizes are written. */
    static const std::map<BarAreaSize, std::pair<int, int>>& getSizeToMinMaxValueForPeriod() noexcept;
    /** @return the minimum size that contains the given period. */
    static BarAreaSize getMinimumSizeForPeriod(int period) noexcept;

    /** @return a map linking the min/max values for all the area size, for the octave in arpeggio. All the sizes are written. */
    static const std::map<BarAreaSize, std::pair<int, int>>& getSizeToMinMaxValueForArpeggioOctave() noexcept;
    /** @return the min/max values for the arpeggio octave and the given bar area size. */
    static std::pair<int, int> getArpeggioOctaveMinMax(BarAreaSize barAreaSize) noexcept;
    /** @return the minimum size that contains the given octave. */
    static BarAreaSize getMinimumSizeForArpeggioOctave(int octave) noexcept;

    /** @return the min/max values for this arpeggio note in octave. */
    static std::pair<int, int> getArpeggioNoteInOctaveMinMax() noexcept;
    /** @return the minimum size that contains the given note in octave. */
    static BarAreaSize getMinimumSizeForArpeggioNoteInOctave(bool forceShrinkFull) noexcept;

    /** @return the min/max values for this pitch note and the given bar area size. */
    static std::pair<int, int> getPitchMinMax(BarAreaSize barAreaSize) noexcept;

    /** @return a map linking the min/max values for all the area size, for pitch. All the sizes are written. */
    static const std::map<BarAreaSize, std::pair<int, int>>& getSizeToMinMaxValueForPitch() noexcept;

    /** @return the minimum size that contains the given pitch. */
    static BarAreaSize getMinimumSizeForPitch(int pitch) noexcept;

    /** @return the minimum size for SID activated and Ratio. */
    static BarAreaSize getMinimumSizeForSidIsActivatedAndRatio() noexcept;
    /** @return the minimum size for SID balance percent. */
    static BarAreaSize getMinimumSizeForSidBalancePercent() noexcept;
    /** @return the minimum size for SID low-shelf volume. */
    static BarAreaSize getMinimumSizeForSidLowShelfVolume(int volume) noexcept;
    /** @return the minimum size for SID arpeggio. */
    static BarAreaSize getMinimumSizeForSidArpeggio(bool forceShrinkFull) noexcept;
    /** @return the minimum size for SID pitch. */
    static BarAreaSize getMinimumSizeForSidPitch() noexcept;

private:
    /**
     * @return the minimum size which contains the given value.
     * @param sizeToMinMaxValue a map linking all the sizes to their accepted min/max values.
     * @param value the value (noise, pitch, etc.).
     */
    static BarAreaSize determineMinimumSize(const std::map<BarAreaSize, std::pair<int, int>>& sizeToMinMaxValue, int value) noexcept;

    static const juce::Image emptyImage;
};

}   // namespace arkostracker
