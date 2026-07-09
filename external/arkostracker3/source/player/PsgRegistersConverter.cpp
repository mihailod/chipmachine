#include "PsgRegistersConverter.h"

#include "../utils/NoteUtil.h"
#include "../utils/NumberUtil.h"
#include "../utils/PsgValues.h"
#include "PsgPeriod.h"
#include "channel/ChannelOutputRegisters.h"

namespace arkostracker 
{

const int PsgRegistersConverter::defaultRatio = 4;
const int PsgRegistersConverter::defaultHardwareEnvelope = 8;
const int PsgRegistersConverter::ratioShiftTolerance = 2;
const int PsgRegistersConverter::thresholdBenDaglishEffect = 10;

PsgRegistersConverter::PsgRegistersConverter(const int pSourcePsgFrequencyHz, const int pTargetPsgFrequencyHz, const float pReferenceFrequencyHz) noexcept :
        sourcePsgFrequencyHz(pSourcePsgFrequencyHz),
        targetPsgFrequencyHz(pTargetPsgFrequencyHz),
        psgPeriod(targetPsgFrequencyHz, pReferenceFrequencyHz),
        currentSoftwareBaseNote(),
        currentHardwareBaseNote()
{
}

PsgRegisters PsgRegistersConverter::convertPeriods(const PsgRegisters& inputRegisters, const int sourcePsgFrequencyHz, const int targetPsgFrequencyHz) noexcept
{
    jassert(sourcePsgFrequencyHz > 0);
    jassert(targetPsgFrequencyHz > 0);

    // Same frequency? Then nothing to do.
    if (sourcePsgFrequencyHz == targetPsgFrequencyHz) {
        return inputRegisters;
    }

    auto outputRegisters = inputRegisters;

    // Converts the software periods.
    for (auto channelIndex = 0; channelIndex < PsgValues::channelCountPerPsg; ++channelIndex) {
        const auto softwarePeriod = inputRegisters.getSoftwarePeriod(channelIndex);
        const auto newPeriod = convertSoftwarePeriod(softwarePeriod, sourcePsgFrequencyHz, targetPsgFrequencyHz);
        outputRegisters.setSoftwarePeriod(channelIndex, newPeriod);
    }

    // Converts the hardware period.
    const auto hardwarePeriod = inputRegisters.getHardwarePeriod();
    outputRegisters.setHardwarePeriod(convertHardwarePeriod(hardwarePeriod, sourcePsgFrequencyHz, targetPsgFrequencyHz));

    // Converts the noise.
    const auto noise = inputRegisters.getNoise();
    outputRegisters.setNoise(convertNoise(noise, sourcePsgFrequencyHz, targetPsgFrequencyHz));

    return outputRegisters;
}

int PsgRegistersConverter::convertSoftwarePeriod(const int period, const int sourcePsgFrequencyHz, const int targetPsgFrequencyHz) noexcept
{
    // Grabs the "out of periods" bits, which are useful for YM.
    const auto outsideBits = static_cast<unsigned int>(period) & 0b1111000000000000U;

    // Uses a mask, else additional bits (from YM) may flaw the division.
    const auto newPeriod = convertPeriod(static_cast<int>(static_cast<unsigned int>(period) & PsgValues::maskSoftwarePeriod), PsgValues::maximumSoftwarePeriod,
        sourcePsgFrequencyHz, targetPsgFrequencyHz);

    // Applies back the bits.
    return static_cast<int>(static_cast<unsigned int>(newPeriod) | outsideBits);
}

int PsgRegistersConverter::convertHardwarePeriod(const int period, const int sourcePsgFrequencyHz, const int targetPsgFrequencyHz) noexcept
{
    // Uses a mask, else additional bits (from YM) may flaw the division.
    return convertPeriod(static_cast<int>(static_cast<unsigned int>(period) & PsgValues::maskHardwarePeriod), PsgValues::maximumHardwarePeriod,
        sourcePsgFrequencyHz, targetPsgFrequencyHz);
}

int PsgRegistersConverter::convertNoise(const int noise, const int sourcePsgFrequencyHz, const int targetPsgFrequencyHz) noexcept
{
    // Uses a mask, else additional bits (from YM) may flaw the division.
    return convertPeriod(static_cast<int>(static_cast<unsigned int>(noise) & PsgValues::maskNoise), PsgValues::maximumNoise, sourcePsgFrequencyHz, targetPsgFrequencyHz);
}

int PsgRegistersConverter::convertPeriod(int period, const int maximumValue, const int sourcePsgFrequencyHz, const int targetPsgFrequencyHz) noexcept
{
    jassert(sourcePsgFrequencyHz > 0);
    jassert(targetPsgFrequencyHz > 0);

    constexpr auto minimumValue = 1;

    // Period is 0? Don't modify it.
    if (period == 0) {
        return period;
    }

    // Same frequency? Then the period doesn't need to change.
    if (sourcePsgFrequencyHz == targetPsgFrequencyHz) {
        return period;
    }

    const auto ratio = static_cast<double>(targetPsgFrequencyHz) / sourcePsgFrequencyHz;
    period = static_cast<int>(period * ratio);

    return NumberUtil::correctNumber(period, minimumValue, maximumValue);
}

std::vector<PsgInstrumentCell> PsgRegistersConverter::encodeAsInstrumentCells(const std::vector<PsgRegisters>& registerList, const int channelIndex, const bool encodeAsForcedPeriods) noexcept
{
    jassert(!registerList.empty());

    std::vector<PsgInstrumentCell> generatedCells;

    // Browses the list of the registers.
    currentSoftwareBaseNote = -1;
    currentHardwareBaseNote = -1;
    for (auto inputPsgRegisters : registerList) {
        // Some YM have strange bits added, which are problematic for the export.
        inputPsgRegisters.correct();

        // First, converts the Registers to the new frequencies.
        auto psgRegisters = convertPeriods(inputPsgRegisters, sourcePsgFrequencyHz, targetPsgFrequencyHz);

        // Special cases specific to AT instrument:
        // - If the software period is 0, it is forced to 1, else it will be encoded at "auto".
        if (psgRegisters.getSoftwarePeriod(channelIndex) == 0) {
            psgRegisters.setSoftwarePeriod(channelIndex, 1);
        }
        // - If the noise channel is open, a noise of 0 should be set to 1, else it won't be heard.
        if (psgRegisters.getMixerNoiseState(channelIndex) && (psgRegisters.getNoise() == 0)) {
            psgRegisters.setNoise(1);
        }

        // Extracts the channel data from the registers.
        const ChannelOutputRegisters channelRegisters(channelIndex, psgRegisters);

        if (encodeAsForcedPeriods) {
            generatedCells.push_back(encodeAsCellForcePeriods(channelRegisters));
        } else {
            generatedCells.push_back(encodeAsCellNotesAndShifts(channelRegisters));
        }
    }

    return generatedCells;
}

PsgInstrumentCell PsgRegistersConverter::encodeAsCellForcePeriods(const ChannelOutputRegisters& channelRegisters) noexcept
{
    switch (channelRegisters.getSoundType()) {
        case SoundType::softwareOnly:
            return { PsgInstrumentCellLink::softOnly, channelRegisters.getVolume(), channelRegisters.getNoise(),
                     channelRegisters.getSoftwarePeriod(), 0, 0, 0,
                     defaultRatio, 0, 0, 0, 0,
                     defaultHardwareEnvelope, false,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
        case SoundType::hardwareOnly:
            return { PsgInstrumentCellLink::hardOnly, 0, channelRegisters.getNoise(),
                     channelRegisters.getHardwarePeriod(), 0, 0, 0,
                     defaultRatio, 0,
                     0, 0, 0,
                     channelRegisters.getHardwareEnvelope(), channelRegisters.isRetrig(),
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
        case SoundType::softwareAndHardware:
            return { PsgInstrumentCellLink::softAndHard, 0, channelRegisters.getNoise(),
                     channelRegisters.getSoftwarePeriod(), 0, 0, 0,
                     defaultRatio, channelRegisters.getHardwarePeriod(),
                     0, 0, 0,
                     channelRegisters.getHardwareEnvelope(), channelRegisters.isRetrig(),
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
        default:
            jassertfalse;
        case SoundType::noSoftwareNoHardware:
            return { PsgInstrumentCellLink::noSoftNoHard, channelRegisters.getVolume(), channelRegisters.getNoise(),
                     0, 0, 0, 0,
                     defaultRatio, 0, 0, 0, 0,
                     defaultHardwareEnvelope, false,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
    }}

PsgInstrumentCell PsgRegistersConverter::encodeAsCellNotesAndShifts(const ChannelOutputRegisters& channelRegisters) noexcept
{
    switch (channelRegisters.getSoundType()) {
        default:
            jassertfalse;
        case SoundType::noSoftwareNoHardware:
            return { PsgInstrumentCellLink::noSoftNoHard, channelRegisters.getVolume(), channelRegisters.getNoise(),
                     0, 0, 0, 0,
                     defaultRatio, 0,
                     0, 0, 0, defaultHardwareEnvelope, false,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
        case SoundType::softwareOnly: {
            const auto softwarePeriod = channelRegisters.getSoftwarePeriod();
            auto noteAndShift = findNoteAndShiftToEncode(softwarePeriod, true);
            const auto [noteInOctave, octave] = noteAndShift.getNoteInOctaveAndOctave();
            return { PsgInstrumentCellLink::softOnly, channelRegisters.getVolume(), channelRegisters.getNoise(),
                     0, noteInOctave, octave, noteAndShift.getShift(),
                     defaultRatio, 0,                     0, 0, 0,
                     defaultHardwareEnvelope, false,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
           };
        }
        case SoundType::hardwareOnly: {
            const auto hardwarePeriod = channelRegisters.getHardwarePeriod();
            auto noteAndShift = findNoteAndShiftToEncode(hardwarePeriod, false);
            const auto [noteInOctave, octave] = noteAndShift.getNoteInOctaveAndOctave();
            return { PsgInstrumentCellLink::hardOnly, 0, channelRegisters.getNoise(),
                     0, noteInOctave, octave, noteAndShift.getShift(),
                     defaultRatio, 0, 0, 0, 0,
                     channelRegisters.getHardwareEnvelope(), channelRegisters.isRetrig(),
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch

            };
        }
        case SoundType::softwareAndHardware: {
            return generateInstrumentCellFromSoftwareAndHardwareChannelRegisters(channelRegisters);
        }
    }
}

NoteAndShift PsgRegistersConverter::findNoteAndShiftToEncode(const int period, const bool software) noexcept
{
    const auto noteAndShift = psgPeriod.findNoteAndShift(period);
    // Is there a base note? If not, the found note becomes the base note.
    auto& currentBaseNote = software ? currentSoftwareBaseNote : currentHardwareBaseNote;
    if (currentBaseNote < 0) {
        currentBaseNote = noteAndShift.getRelativeNote();
    }
    // The note to encode is the found note, minus the base note.
    // It becomes the Arpeggio! Thus, for the first note, the arp is 0.
    const auto noteToEncode = noteAndShift.getRelativeNote() - currentBaseNote;

    return { noteToEncode, noteAndShift.getShift() };
}

PsgInstrumentCell PsgRegistersConverter::generateInstrumentCellFromSoftwareAndHardwareChannelRegisters(const ChannelOutputRegisters& registers) noexcept
{
    jassert((registers.getSoundType() == SoundType::softwareAndHardware));

    const auto softwarePeriod = registers.getSoftwarePeriod();
    const auto hardwarePeriod = registers.getHardwarePeriod();
    const auto softwareNoteAndShift = findNoteAndShiftToEncode(softwarePeriod, true);
    const auto hardwareNoteAndShift = findNoteAndShiftToEncode(hardwarePeriod, false);

    const auto [link, ratio] = findLink(registers);
    constexpr auto volume = 0;           // The volume is hardware, there is no need.
    const auto noise = registers.getNoise();

    const auto hardwareEnvelope = registers.getHardwareEnvelope();
    const auto retrig = registers.isRetrig();

    switch (link) {
        case PsgInstrumentCellLink::softToHard: {
            const auto [noteInOctave, octave] = softwareNoteAndShift.getNoteInOctaveAndOctave();
            return { link, volume, noise, 0, noteInOctave, octave, softwareNoteAndShift.getShift(), ratio,
                     0, 0, 0, 0, hardwareEnvelope, retrig,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
        }
        case PsgInstrumentCellLink::hardToSoft: {
            const auto [noteInOctave, octave] = hardwareNoteAndShift.getNoteInOctaveAndOctave();
            return { link, volume, noise, 0, noteInOctave, octave, hardwareNoteAndShift.getShift(), ratio,
                     0, 0, 0, 0,
                     hardwareEnvelope, retrig,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch

            };
        }
        case PsgInstrumentCellLink::hardOnly:
        case PsgInstrumentCellLink::noSoftNoHard:
        case PsgInstrumentCellLink::softOnly:
        default:
            jassertfalse;
        case PsgInstrumentCellLink::softAndHard:
            // In case the hardware period is below a certain arbitrary threshold, consider it a "Ben Daglish effect".
            if (hardwarePeriod < thresholdBenDaglishEffect) {
                const auto [noteInOctave, octave] = softwareNoteAndShift.getNoteInOctaveAndOctave();
                return { link, volume, noise, 0, noteInOctave, octave, softwareNoteAndShift.getShift(), defaultRatio,
                         hardwarePeriod, 0, 0, 0, hardwareEnvelope, retrig,
                         false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                         PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch

                };
            }
            // Normal behaviour. Note that the hardware note is always encoded relative to the software one.
            // We consider the software note to be the "lead". This is only a convention.
            const auto hardwareArpeggio = psgPeriod.findNoteAndShift(hardwarePeriod).getRelativeNote() - psgPeriod.findNoteAndShift(softwarePeriod).getRelativeNote();
            const auto [hardwareNoteInOctave, hardwareOctave] = NoteUtil::getNoteInOctaveAndOctave(hardwareArpeggio);
            const auto [softwareNoteInOctave, softwareOctave] = softwareNoteAndShift.getNoteInOctaveAndOctave();
            return { link, volume, noise, 0, softwareNoteInOctave, softwareOctave,
                     softwareNoteAndShift.getShift(), defaultRatio,
                     0, hardwareNoteInOctave, hardwareOctave, hardwareNoteAndShift.getShift(),
                     hardwareEnvelope, retrig,
                     false, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                     PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
            };
    }
}

std::pair<PsgInstrumentCellLink, int> PsgRegistersConverter::findLink(const ChannelOutputRegisters& registers) noexcept
{
    jassert((registers.getSoundType() == SoundType::softwareAndHardware));

    // Extracts the data for analysis.
    const auto softwarePeriod = registers.getSoftwarePeriod();
    const auto hardwarePeriod = registers.getHardwarePeriod();
    const auto softToHardRatioAndShift = findRatioAndShiftSoftToHard(softwarePeriod, hardwarePeriod);
    const auto hardToSoftRatioAndShift = findRatioAndShiftHardToSoft(softwarePeriod, hardwarePeriod);

    const auto softwareNotePerfect = (findNoteAndShiftToEncode(softwarePeriod, true).getShift() == 0);
    //const auto hardwareNotePerfect = (findNoteAndShiftToEncode(hardwarePeriod, false).shift == 0);

    // What is the type to choose?

    // If no ratio is found, fallback to SoftAndHard.
    if ((softToHardRatioAndShift.first < 0) && (hardToSoftRatioAndShift.first < 0)) {
        return { PsgInstrumentCellLink::softAndHard, defaultRatio };
    }

    // Maybe SoftToHard if the shift is "perfect", or HardToShift if the shift is "perfect".
    auto softToHardPreferred = (softToHardRatioAndShift.first >= 0);
    auto hardToSoftPreferred = (hardToSoftRatioAndShift.first >= 0);
    // If both are preferred, favored the one with a "perfect" note.
    if (softToHardPreferred && hardToSoftPreferred) {
        if (softwareNotePerfect) {
            hardToSoftPreferred = false;
        } else {
            softToHardPreferred = false;
        }
    }

    if (softToHardPreferred) {
        return { PsgInstrumentCellLink::softToHard, softToHardRatioAndShift.first };
    }
    if (hardToSoftPreferred) {
        return { PsgInstrumentCellLink::hardToSoft, hardToSoftRatioAndShift.first };
    }

    // Neither s2h nor h2s is preferred. Chooses the one which shift is smaller.
    return (softToHardRatioAndShift.second <= hardToSoftRatioAndShift.second)
           ? std::make_pair(PsgInstrumentCellLink::softToHard, softToHardRatioAndShift.first)
           : std::make_pair(PsgInstrumentCellLink::hardToSoft, hardToSoftRatioAndShift.first);
}

std::pair<int, int> PsgRegistersConverter::findRatioAndShiftSoftToHard(const int softwarePeriod, const int hardwarePeriod) noexcept
{
    // The software period must be bigger than/equal to the hardware period, else AT2 can not reproduce it as a ratio.
    if (softwarePeriod < hardwarePeriod) {
        return { -1, 0 };
    }

    auto currentPeriod = softwarePeriod;
    for (auto ratio = 0; ratio < 8; ++ratio) {
        const auto shift = (currentPeriod - hardwarePeriod);
        if (abs(shift) <= ratioShiftTolerance) {     // A small shift is tolerated.
            return { ratio, shift };
        }
        // Not the right ratio. Next one.
        currentPeriod /= 2;
    }

    return { -1, 0 };      // Not found!
}

std::pair<int, int> PsgRegistersConverter::findRatioAndShiftHardToSoft(const int softwarePeriod, const int hardwarePeriod) noexcept
{
    // The software period must be bigger than/equal to the hardware period, else AT2 can not reproduce it as a ratio.
    if (softwarePeriod < hardwarePeriod) {
        return { -1, 0 };
    }

    auto currentPeriod = hardwarePeriod;
    for (auto ratio = 0; ratio < 8; ++ratio) {
        const auto shift = (currentPeriod - softwarePeriod);
        if (abs(shift) <= ratioShiftTolerance) {     // A small shift is tolerated.
            return { ratio, shift };
        }
        // Not the right ratio. Next one.
        currentPeriod *= 2;
    }

    return { -1, 0 };      // Not found!
}

}   // namespace arkostracker
