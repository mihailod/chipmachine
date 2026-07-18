#include "PsgInstrumentCellEncoder.h"

#include "../../sourceGenerator/SourceGenerator.h"
#include "../../../song/instrument/psg/PsgInstrumentCell.h"
#include "../../../utils/PsgValues.h"
#include "../../../utils/StringUtil.h"
#include "../../../utils/BitNumber.h"
#include "../../../song/instrument/psg/PsgPart.h"
#include "../../playerConfiguration/PlayerConfiguration.h"

namespace arkostracker 
{

PsgInstrumentCellEncoder::PsgInstrumentCellEncoder(SourceGenerator& pSourceGenerator) noexcept :
        sourceGenerator(pSourceGenerator)
{
}

void PsgInstrumentCellEncoder::encodePsgInstrumentCell(PlayerConfiguration& playerConfiguration, const PsgPart& psgPart, int cellIndex) noexcept
{
    const auto psgInstrumentCell = psgPart.buildLowLevelCell(cellIndex);

    switch (psgInstrumentCell.getLink()) {
        case PsgInstrumentCellLink::softOnly:
            encodeSoftOnly(playerConfiguration, psgInstrumentCell);
            break;
        case PsgInstrumentCellLink::noSoftNoHard:
            encodeNoSoftNoHard(playerConfiguration, psgInstrumentCell);
            break;
        case PsgInstrumentCellLink::hardOnly:
            encodeHardOnly(playerConfiguration, psgInstrumentCell);
            break;
        case PsgInstrumentCellLink::softToHard:
            encodeSoftToHard(playerConfiguration, psgInstrumentCell);
            break;
        case PsgInstrumentCellLink::hardToSoft:
            encodeHardToSoft(playerConfiguration, psgInstrumentCell);
            break;
        case PsgInstrumentCellLink::softAndHard:
            encodeSoftAndHard(playerConfiguration, psgInstrumentCell);
            break;
        default:
            jassertfalse;       // Shouldn't happen.
            break;
    }
    sourceGenerator.addEmptyLine();
}

void PsgInstrumentCellEncoder::encodeNoSoftNoHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept
{
    playerConfiguration.addFlag(PlayerConfigurationFlag::noSoftNoHard);

    // Note: this method may be called in case of a SoftOnly with a volume to 0.
    const auto volume = cell.getVolume();
    // Optimization: if volume is 0, don't encode the noise (no one should care about a possible noise change?).
    const auto noise = (volume == 0) ? 0 : cell.getNoise();
    const auto isNoise = (noise > 0);

    jassert((noise >= 0) && (noise <= 0x1f));
    jassert((volume >= 0) && (volume <= 0xf));

    // First byte: noise? (b0), volume (b1-b4), tag.
    BitNumber firstByte;
    firstByte.injectNumber(linkNoSoftNoHardTag, 3U);
    firstByte.injectNumber(volume, 4);
    firstByte.injectBool(isNoise);

    const auto comment = "No Soft no Hard. Volume: " + juce::String(volume) + ". Noise? " + StringUtil::boolToString(isNoise) + ".";
    sourceGenerator.declareByte(firstByte.get(), comment);

    // Encodes the noise, if any.
    if (isNoise) {
        playerConfiguration.addFlag(PlayerConfigurationFlag::noSoftNoHardNoise);

        sourceGenerator.declareByte(noise, "Noise: " + juce::String(noise) + ".");
    }
}

void PsgInstrumentCellEncoder::encodeSoftOnly(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& psgInstrumentCell) noexcept
{
    // Corner case: if volume is 0, it is better to encode it as a NoSoftNoHard (no one should care about a possible noise change?).
    const auto volume = psgInstrumentCell.getVolume();
    if (volume == 0) {
        encodeNoSoftNoHard(playerConfiguration, psgInstrumentCell);
        return;
    }

    const auto noise = psgInstrumentCell.getNoise();
    const auto pitch = psgInstrumentCell.getSoftwarePitch();
    const auto arpeggio = psgInstrumentCell.getSoftwareArpeggio();
    const auto period = psgInstrumentCell.getSoftwarePeriod();
    const auto isNoise = (noise > 0);
    const auto isPitch = (pitch != 0);
    const auto isArpeggio = (arpeggio != 0);
    const auto isForcedPeriod = (period != 0);
    jassert((noise >= 0) && (noise <= 0x1f));
    jassert((volume >= 0) && (volume <= 0xf));
    // Bit 0 = volume only? This means no noise, no pitch, no arpeggio, and automatic period.
    const auto volumeOnly = !isNoise && !isPitch && !isArpeggio && !isForcedPeriod;

    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareOnly);
    playerConfiguration.addFlag(isNoise, PlayerConfigurationFlag::softwareOnlyNoise);
    playerConfiguration.addFlag(isArpeggio, PlayerConfigurationFlag::softwareOnlySoftwareArpeggio);
    playerConfiguration.addFlag(isPitch, PlayerConfigurationFlag::softwareOnlySoftwarePitch);
    playerConfiguration.addFlag(isForcedPeriod, PlayerConfigurationFlag::softwareOnlyForcedSoftwarePeriod);

    // First byte: link tag (b0-b2), volume (b3-b6), volumeOnly? (b7).
    BitNumber firstByte;
    firstByte.injectNumber(linkSoftOnlyTag, 3);
    firstByte.injectNumber(volume, 4);
    firstByte.injectBool(volumeOnly);

    auto comment = "Soft only. Volume: " + juce::String(volume) + ".";
    if (volumeOnly) {
        comment += " Volume only.";
    }
    sourceGenerator.declareByte(firstByte.get(), comment);

    // If volume only, we can stop here.
    if (volumeOnly) {
        return;
    }

    // Encodes another byte containing what is present.
    // Noise (b0-4), pitch? (b5), arpeggio? (b6), forced software auto? (b7. 0=auto).
    comment = "Additional data. Noise: " + juce::String(noise) + ". Pitch? " + StringUtil::boolToString(isPitch) + ". Arp? "
              + StringUtil::boolToString(isArpeggio) + ". Period? " + StringUtil::boolToString(isForcedPeriod) + ".";

    BitNumber secondByte;
    secondByte.injectNumber(noise, 5);
    secondByte.injectBool(isPitch);
    secondByte.injectBool(isArpeggio);
    secondByte.injectBool(isForcedPeriod);

    sourceGenerator.declareByte(secondByte.get(), comment);

    // Encodes the period, or the arpeggio and pitch.
    encodePeriodOrPitchAndArpeggio(period, arpeggio, pitch);
}

void PsgInstrumentCellEncoder::encodePeriodOrPitchAndArpeggio(int period, int arpeggio, int pitch) noexcept
{
    // Forced period? If yes, no pitch or arpeggio.
    if (period != 0) {
        sourceGenerator.declareWord(period, "Forced period.");
    } else {
        if (arpeggio != 0) {
            sourceGenerator.declareByte(arpeggio, "Arpeggio.");
        }
        if (pitch != 0) {
            sourceGenerator.declareWord(-pitch, "Pitch.");     // An inverted pitch (period based) is encoded.
        }
    }
}

void PsgInstrumentCellEncoder::encodeHardOnly(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept
{
    const auto noise = cell.getNoise();
    const auto pitch = cell.getHardwarePitch();
    const auto arpeggio = cell.getHardwareArpeggio();
    const auto period = cell.getHardwarePeriod();
    const auto isRetrig = cell.isRetrig();
    const auto envelope = cell.getHardwareEnvelope();
    const auto isNoise = (noise > 0);
    const auto isPitch = (pitch != 0);
    const auto isArpeggio = (arpeggio != 0);
    const auto isForcedPeriod = (period != 0);
    jassert((noise >= 0) && (noise <= 0x1f));
    jassert((envelope >= 8) && (envelope <= 15));

    playerConfiguration.addFlag(PlayerConfigurationFlag::hardwareOnly);
    playerConfiguration.addFlag(isRetrig, PlayerConfigurationFlag::hardwareOnlyRetrig);
    playerConfiguration.addFlag(isNoise, PlayerConfigurationFlag::hardwareOnlyNoise);
    playerConfiguration.addFlag(isArpeggio, PlayerConfigurationFlag::hardwareOnlyHardwareArpeggio);
    playerConfiguration.addFlag(isPitch, PlayerConfigurationFlag::hardwareOnlyHardwarePitch);
    playerConfiguration.addFlag(isForcedPeriod, PlayerConfigurationFlag::hardwareOnlyForcedHardwarePeriod);

    // Simple if no arpeggio, no noise, no pitch, auto period.
    const auto isSimple = !isArpeggio && !isPitch && !isForcedPeriod && !isNoise;

    // First byte. b3: retrig?, b4-b6: envelope, b7: simple?
    BitNumber firstByte;
    firstByte.injectNumber(linkHardOnlyTag, 3);
    firstByte.injectBool(isRetrig);
    firstByte.injectNumber(envelope - 8, 3);
    firstByte.injectBool(isSimple);

    auto comment = "Hard only. Envelope: " + juce::String(envelope) + ". Retrig? " + StringUtil::boolToString(isRetrig) + ".";
    if (isSimple) {
        comment += " Simple configuration.";
    } else {
        comment += " Not simple configuration.";
    }
    sourceGenerator.declareByte(firstByte.get(), comment);

    // If simple, nothing else to do.
    if (isSimple) {
        return;
    }

    // Second byte. b0-b4: noise, b5: pitch?, b6: arpeggio?, b7: forced period?
    BitNumber secondByte;
    secondByte.injectNumber(noise, 5);
    secondByte.injectBool(isPitch);
    secondByte.injectBool(isArpeggio);
    secondByte.injectBool(isForcedPeriod);

    comment = "Additional data. Noise: " + juce::String(noise) + ". Pitch? " + StringUtil::boolToString(isPitch) + ". Arp? "
              + StringUtil::boolToString(isArpeggio) + ". Period? " + StringUtil::boolToString(isForcedPeriod) + ".";
    sourceGenerator.declareByte(secondByte.get(), comment);

    // Encodes the period, or the arpeggio and pitch.
    encodePeriodOrPitchAndArpeggio(period, arpeggio, pitch);
}

void PsgInstrumentCellEncoder::encodeSoftToHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept
{
    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareToHardware);

    // The source is the software part, the destination the hardware part.
    const auto softwarePitch = cell.getSoftwarePitch();
    const auto softwareArpeggio = cell.getSoftwareArpeggio();
    const auto softwarePeriod = cell.getSoftwarePeriod();
    const auto hardwarePitchShift = cell.getHardwarePitch();
    encodeSoftToHardOrHardToSoft(playerConfiguration, true, softwarePeriod, softwareArpeggio, softwarePitch, hardwarePitchShift, cell);
}

void PsgInstrumentCellEncoder::encodeHardToSoft(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept
{
    playerConfiguration.addFlag(PlayerConfigurationFlag::hardwareToSoftware);

    // The source is the hardware part, the destination the software part.
    const auto hardwarePitch = cell.getHardwarePitch();
    const auto hardwareArpeggio = cell.getHardwareArpeggio();
    const auto hardwarePeriod = cell.getHardwarePeriod();
    const auto softwarePitchShift = cell.getSoftwarePitch();
    encodeSoftToHardOrHardToSoft(playerConfiguration, false, hardwarePeriod, hardwareArpeggio, hardwarePitch, softwarePitchShift, cell);
}

void PsgInstrumentCellEncoder::encodeSoftToHardOrHardToSoft(PlayerConfiguration& playerConfiguration, bool isSoftToHard,
                                                            int sourcePeriod, int sourceArpeggio, int sourcePitch,
                                                            int destinationPitchShift, const LowLevelPsgInstrumentCell& cell) noexcept
{
    const auto softToHard = (cell.getLink() == PsgInstrumentCellLink::softToHard);
    jassert((cell.getLink() == PsgInstrumentCellLink::softToHard) || (cell.getLink() == PsgInstrumentCellLink::hardToSoft));

    const auto noise = cell.getNoise();
    const auto envelope = cell.getHardwareEnvelope();
    const auto ratio = cell.getRatio();
    const auto isRetrig = cell.isRetrig();

    const auto isNoise = (noise > 0);
    const auto isSourcePitch = (sourcePitch != 0);
    const auto isSourceArpeggio = (sourceArpeggio != 0);
    const auto isSourceForcedPeriod = (sourcePeriod != 0);
    const auto isDestinationPitchShift = (destinationPitchShift != 0);

    playerConfiguration.addFlag(isRetrig, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareRetrig : PlayerConfigurationFlag::hardwareToSoftwareRetrig);
    playerConfiguration.addFlag(isNoise, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareNoise : PlayerConfigurationFlag::hardwareToSoftwareNoise);
    playerConfiguration.addFlag(isSourcePitch, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareSoftwarePitch : PlayerConfigurationFlag::hardwareToSoftwareHardwarePitch);
    playerConfiguration.addFlag(isSourceArpeggio, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareSoftwareArpeggio : PlayerConfigurationFlag::hardwareToSoftwareHardwareArpeggio);
    playerConfiguration.addFlag(isSourceForcedPeriod, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareForcedSoftwarePeriod : PlayerConfigurationFlag::hardwareToSoftwareForcedHardwarePeriod);
    playerConfiguration.addFlag(isDestinationPitchShift, isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareHardwarePitch : PlayerConfigurationFlag::hardwareToSoftwareSoftwarePitch);

    jassert((noise >= 0) && (noise <= 0x1f));
    jassert((envelope >= 8) && (envelope <= 15));
    jassert((ratio >= 0) && (ratio <= 7));

    // First byte. b0: retrig?, b1: noise?, b2-b4: envelope, b5-b7: link tag.
    BitNumber firstByte;
    firstByte.injectNumber(softToHard ? linkSoftToHardTag : linkHardToSoftTag, 3);
    firstByte.injectBool(isRetrig);
    firstByte.injectNumber(envelope - 8, 3);
    firstByte.injectBool(isNoise);

    auto comment = juce::String(softToHard ? "Soft to Hard" : "Hard to Soft");
    comment += ". Envelope: " + juce::String(envelope) + ". Retrig? " + StringUtil::boolToString(isRetrig) + ". Noise? " + StringUtil::boolToString(isNoise) + ".";
    sourceGenerator.declareByte(firstByte.get(), comment);

    // Noise?
    if (isNoise) {
        sourceGenerator.declareByte(noise, "Noise.");
    }

    // Second byte. b0-b2: inverted ratio, b3: hardware pitch shift?, b4: software pitch?, b5: software arpeggio?, b6: forced software period?, b7: simple?
    // Simple if no software arpeggio, no software pitch, auto software period, no hardware pitch shift.
    const auto isSimple = !isSourceArpeggio && !isSourcePitch && !isSourceForcedPeriod && !isDestinationPitchShift;
    BitNumber secondByte;
    secondByte.injectNumber(7 - ratio, 3);
    if (isSimple) {
        secondByte.injectNumber(0, 4);      // Useless bits if simple.
    } else {
        secondByte.injectBool(isDestinationPitchShift);
        secondByte.injectBool(isSourcePitch);
        secondByte.injectBool(isSourceArpeggio);
        secondByte.injectBool(isSourceForcedPeriod);
    }
    secondByte.injectBool(isSimple);

    comment = isSimple ? "Simple case." : "Complex case.";
    comment += " Ratio: " + juce::String(ratio);
    sourceGenerator.declareByte(secondByte.get(), comment);

    // Nothing else to do if simple.
    if (isSimple) {
        return;
    }

    // Encodes the software forced period, or the arpeggio and/or shift.
    encodePeriodOrPitchAndArpeggio(sourcePeriod, sourceArpeggio, sourcePitch);

    // Hardware pitch shift, if any. Like all Pitches, its inverted value is encoded!
    if (isDestinationPitchShift) {
        sourceGenerator.declareWord(-destinationPitchShift,  "Destination pitch shift.");
    }
}

void PsgInstrumentCellEncoder::encodeSoftAndHard(PlayerConfiguration& playerConfiguration, const LowLevelPsgInstrumentCell& cell) noexcept
{
    const auto noise = cell.getNoise();
    const auto envelope = cell.getHardwareEnvelope();
    const auto isRetrig = cell.isRetrig();

    const auto isNoise = (noise > 0);

    playerConfiguration.addFlag(PlayerConfigurationFlag::softwareAndHardware);
    playerConfiguration.addFlag(isRetrig, PlayerConfigurationFlag::softwareAndHardwareRetrig);
    playerConfiguration.addFlag(isNoise, PlayerConfigurationFlag::softwareAndHardwareNoise);

    jassert((noise >= PsgValues::minimumNoise) && (noise <= PsgValues::maximumNoise));
    jassert((envelope >= 8) && (envelope <= PsgValues::maximumHardwareEnvelope));

    // First byte. b0-b2: link tag, b3: retrig?, b4-b6: envelope, b7: noise?.
    BitNumber firstByte;
    firstByte.injectNumber(linkSoftAndHardTag, 3);
    firstByte.injectBool(isRetrig);
    firstByte.injectNumber(envelope - 8, 3);
    firstByte.injectBool(isNoise);

    auto comment = juce::String("Soft and Hard");
    comment += ". Envelope: " + juce::String(envelope) + ". Retrig? " + StringUtil::boolToString(isRetrig) + ". Noise? " + StringUtil::boolToString(isNoise) + ".";
    sourceGenerator.declareByte(firstByte.get(), comment);

    // Noise?
    if (isNoise) {
        sourceGenerator.declareByte(noise, "Noise.");
    }

    // Second byte.
    /*
        7  6  5  4  3  2  1  0
        sh fh ha hp ss fs sa sp
        ---hard---- ---soft----

        sh = simple hardware part?
        fh = forced hardware period?
        ha = hardware arpeggio?
        hp = hardware pitch?
        ss = simple software part?
        fs = forced software period?
        sa = software arpeggio?
        sp = software pitch?
    */

    // Software part.
    const auto softwarePeriod = cell.getSoftwarePeriod();
    const auto softwareArpeggio = cell.getSoftwareArpeggio();
    const auto softwarePitch = cell.getSoftwarePitch();

    const auto isSoftwareForcedPeriod = (softwarePeriod != 0);
    const auto isSoftwarePitch = (softwarePitch != 0);
    const auto isSoftwareArpeggio = (softwareArpeggio != 0);

    const auto isSimpleSoftware = !isSoftwareForcedPeriod && !isSoftwarePitch && !isSoftwareArpeggio;

    playerConfiguration.addFlag(isSoftwareArpeggio, PlayerConfigurationFlag::softwareAndHardwareSoftwareArpeggio);
    playerConfiguration.addFlag(isSoftwarePitch, PlayerConfigurationFlag::softwareAndHardwareSoftwarePitch);
    playerConfiguration.addFlag(isSoftwareForcedPeriod, PlayerConfigurationFlag::softwareAndHardwareForcedSoftwarePeriod);

    // Hardware part.
    const auto hardwarePeriod = cell.getHardwarePeriod();
    const auto hardwareArpeggio = cell.getHardwareArpeggio();
    const auto hardwarePitch = cell.getHardwarePitch();

    const auto isHardwareForcedPeriod = (hardwarePeriod != 0);
    const auto isHardwarePitch = (hardwarePitch != 0);
    const auto isHardwareArpeggio = (hardwareArpeggio != 0);

    const auto isSimpleHardware = !isHardwareForcedPeriod && !isHardwarePitch && !isHardwareArpeggio;

    playerConfiguration.addFlag(isHardwareArpeggio, PlayerConfigurationFlag::softwareAndHardwareHardwareArpeggio);
    playerConfiguration.addFlag(isHardwarePitch, PlayerConfigurationFlag::softwareAndHardwareHardwarePitch);
    playerConfiguration.addFlag(isHardwareForcedPeriod, PlayerConfigurationFlag::softwareAndHardwareForcedHardwarePeriod);

    // Agglomerate.
    BitNumber secondByte;
    // Software part.
    secondByte.injectBool(isSoftwarePitch);
    secondByte.injectBool(isSoftwareArpeggio);
    secondByte.injectBool(isSoftwareForcedPeriod);
    secondByte.injectBool(isSimpleSoftware);
    // Hardware part.
    secondByte.injectBool(isHardwarePitch);
    secondByte.injectBool(isHardwareArpeggio);
    secondByte.injectBool(isHardwareForcedPeriod);
    secondByte.injectBool(isSimpleHardware);

    comment = "Second byte. Simple Soft? " + StringUtil::boolToString(isSimpleSoftware) + ". Simple Hard? " + StringUtil::boolToString(isSimpleHardware) + ".";
    sourceGenerator.declareByte(secondByte.get(), comment);

    // Additional data, if needed. The hardware part is encoded first.
    if (!isSimpleHardware) {
        // Encodes the forced period, or the arpeggio/pitch for the Hardware part.
        encodePeriodOrPitchAndArpeggio(hardwarePeriod, hardwareArpeggio, hardwarePitch);
    }
    if (!isSimpleSoftware) {
        // Encodes the forced period, or the arpeggio/pitch for the Software part.
        encodePeriodOrPitchAndArpeggio(softwarePeriod, softwareArpeggio, softwarePitch);
    }
}

void PsgInstrumentCellEncoder::encodeLoopToSilence() noexcept
{
    sourceGenerator.declareByte(linkLoopToSilence, "Loop to silence.");
}

void PsgInstrumentCellEncoder::encodeLoopToLabel(const juce::String& label) noexcept
{
    sourceGenerator.declareByte(linkLoop, "Loop.");
    sourceGenerator.declareWordForceReference(label, "Loop here.");
}

}   // namespace arkostracker

