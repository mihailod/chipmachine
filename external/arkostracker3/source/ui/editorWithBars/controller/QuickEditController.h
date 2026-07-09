#pragma once

#include "../view/AreaType.h"
#include "../view/manualEdit/BaseManualEditDialog.h"

namespace arkostracker
{

class Id;

/** Base controller to manage the quick edit (show Dialogs, and manage inputs). This could be used by any Bar Editor (PSG, Expressions). */
class QuickEditController
{
public:
    /** Constructor. */
    QuickEditController() = default;

    /** Destructor. */
    virtual ~QuickEditController() = default;

    /**
     * The user wants to edit an area type. Opens the right Dialog, will modify the Instrument if needed, notifies.
     * @param itemId the ID of the item (instrument, etc.).
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     */
    void edit(const Id& itemId, AreaType areaType, int barIndex) noexcept;

protected:
    /**
     * @return the length of the item.
     * @param itemId the ID of th item (instrument, etc.).
     */
    virtual int getItemLength(const Id& itemId) const noexcept = 0;

    /**
     * Validates or not the given noise values. If valid, modifies the Instrument.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateNoise(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given envelope values. If valid, modifies the Instrument.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateEnvelope(const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given sound type values. If valid, modifies the Instrument.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSoundType(const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given SID activation and ratio values. If valid, modifies the Instrument.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSidIsActivatedAndRatio(const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given pitch values. If valid, modifies the Instrument.
     * @param isPrimaryPitch true if primary pitch, false if secondary.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validatePitch(bool isPrimaryPitch, const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given arpeggio in note values. If valid, modifies the Instrument.
     * @param isPrimaryArpeggioInNote true if primary arpeggio in note, false if secondary.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateArpeggioNoteInOctave(bool isPrimaryArpeggioInNote, const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given arpeggio octave values. If valid, modifies the Instrument.
     * @param isPrimaryArpeggioOctave true if primary arpeggio octave, false if secondary.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateArpeggioOctave(bool isPrimaryArpeggioOctave, const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given period values. If valid, modifies the Instrument.
     * @param isPrimaryPeriod true if primary period, false if secondary.
     * @param itemId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validatePeriod(bool isPrimaryPeriod, const Id& itemId, int barIndex, const juce::String& values) const noexcept = 0;


    // ===============================================================================

    /**
     * Validates or not the given SID low-shelf volumes. If valid, modifies the Instrument.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSidLowShelfVolume(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given SID balance percent. If valid, modifies the Instrument.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSidBalancePercent(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given SID arpeggio. If valid, modifies the Instrument.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSidArpeggio(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept = 0;

    /**
     * Validates or not the given SID pitch. If valid, modifies the Instrument.
     * @param instrumentId the ID of the item (instrument, etc.).
     * @param barIndex the index of the bar.
     * @param values the raw values ("1 1f 5,9, aaaa").
     * @return a String of human-readable error message, if not validated.
     */
    virtual juce::String validateSidPitch(const Id& instrumentId, int barIndex, const juce::String& values) const noexcept = 0;

    // ===============================================================================

    /**
     * @return splits from the given values, which are comma or space separated.
     * @param values the values.
     */
    static std::vector<juce::String> normalize(const juce::String& values) noexcept;

    /**
     * Checks the given values in the string.
     * @param values the comma separated values in a String.
     * @param minimumValue the minimum accepted value.
     * @param maximumValue the maximum accepted value.
     * @param isHex true if the values are in hexadecimal, false in decimal.
     * @return a possible error message, and the values if valid.
     */
    static std::pair<juce::String, std::vector<int>> validateValues(const juce::String& values, int minimumValue, int maximumValue, bool isHex = true) noexcept;

    class EnvelopeResult
    {
    public:
        EnvelopeResult(const int pVolume, const int pHardwareEnvelope) :
                volume(pVolume),
                hardwareEnvelope(pHardwareEnvelope)
        {

        }
        int getVolume() const noexcept
        {
            return volume;
        }

        int getHardwareEnvelope() const noexcept
        {
            return hardwareEnvelope;
        }

    private:
        int volume;                 // From 0 to 16.
        int hardwareEnvelope;       // Only relevant if volume is 16.
    };

private:
    /** Called after leaving a dialog. Closes it. */
    void onLeaveDialog() noexcept;

    std::unique_ptr<BaseManualEditDialog> dialog;
};

}   // namespace arkostracker
