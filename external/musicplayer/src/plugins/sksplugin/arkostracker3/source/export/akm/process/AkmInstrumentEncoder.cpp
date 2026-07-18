#include "AkmInstrumentEncoder.h"

#include "../../../song/Song.h"
#include "../../../utils/BitNumber.h"
#include "../../../utils/NumberUtil.h"
#include "../../SongExportResult.h"
#include "../../sourceGenerator/SourceGenerator.h"

namespace arkostracker 
{

AkmInstrumentEncoder::AkmInstrumentEncoder(const Song& pSong, SourceGenerator& pSourceGenerator,
                                           SongExportResult& pSongExportResult, bool pEncodeInvertedRatio,
                                           std::function<juce::String(int instrumentIndex)> pGetInstrumentLabel) noexcept :
        song(pSong),
        sourceGenerator(pSourceGenerator),
        songExportResult(pSongExportResult),
        encodeInvertedRatio(pEncodeInvertedRatio),
        getInstrumentLabel(std::move(pGetInstrumentLabel))
{
}

bool AkmInstrumentEncoder::encodeInstrument(int instrumentIndex) noexcept
{
    // Disark: we consider a Byte Region has been declared. The caller will also close it.

    auto success = true;

    song.performOnConstInstrumentFromIndex(instrumentIndex, [&](const Instrument& instrument) {
        // Makes sure it is a PSG Instrument.
        if (instrument.getType() != InstrumentType::psgInstrument) {
            songExportResult.addError("Instrument " + NumberUtil::signedHexToStringWithPrefix(instrumentIndex) + " is not a PSG instrument.");
            success = false;
            return;
        }
        const auto& psgPart = instrument.getConstPsgPart();

        // Encodes the label.
        const auto finalLabel = getInstrumentLabel(instrumentIndex);
        sourceGenerator.declareLabel(finalLabel);

        // Retrig? There shouldn't.
        if (psgPart.isInstrumentRetrig()) {
            songExportResult.addWarning("Instrument Retrig is not supported in this format.");
        }

        sourceGenerator.declareByte(psgPart.getSpeed(), "Speed.");

        // Encodes each line via the inner class above.
        const auto instrumentLoop = psgPart.getMainLoop();
        const auto endIndex = instrumentLoop.getEndIndex();
        const auto loopStartIndex = instrumentLoop.getStartIndex();
        for (auto cellIndex = 0; cellIndex <= endIndex; ++cellIndex) {
            const auto cell = psgPart.buildLowLevelCell(cellIndex);
            encodeInstrumentCell(cell, instrumentIndex, cellIndex, loopStartIndex);
        }

        // Encodes the end/loop.
        encodeInstrumentEnd(instrumentIndex, instrumentLoop.isLooping(), loopStartIndex);

        sourceGenerator.addEmptyLine();
    });

    return success;
}

void AkmInstrumentEncoder::encodeInstrumentCell(const LowLevelPsgInstrumentCell& cell, int instrumentIndex, int cellIndex, int loopIndex) noexcept
{
    // Should the loop label be encoded? Yes if the Instrument loops here.
    if (cellIndex == loopIndex) {
        const auto loopLabel = getInstrumentLabelWithLoop(instrumentIndex);
        sourceGenerator.declareLabel(loopLabel);
    }

    // Which encoder to call? Depends on the Link.
    switch (cell.getLink()) {
        case PsgInstrumentCellLink::noSoftNoHard:
            encodeInstrumentCellNoSoftNoHard(cell);
            break;
        case PsgInstrumentCellLink::softOnly:
            encodeInstrumentCellSoftwareOnly(cell);
            break;
        case PsgInstrumentCellLink::softToHard:
            encodeInstrumentCellSoftwareToHardware(cell);
            break;
        case PsgInstrumentCellLink::softAndHard:
            encodeInstrumentCellSoftwareAndHardware(cell);
            break;
        case PsgInstrumentCellLink::hardOnly:
            [[fallthrough]];
        case PsgInstrumentCellLink::hardToSoft:
            [[fallthrough]];
        default:
            songExportResult.addWarning("Only No software no Hardware, Software only, Software to Hardware, Software and Hardware are allowed.");
            // Encodes a "no soft no hard" instead, as a fallback.
            encodeInstrumentCellNoSoftNoHard(cell);
            break;
    }

    sourceGenerator.addEmptyLine();
}

void AkmInstrumentEncoder::encodeInstrumentCellNoSoftNoHard(const LowLevelPsgInstrumentCell& cell) noexcept
{
    //jassert(cell.getLink() == PsgInstrumentCellLink::NoSoftNoHard);   --> No, because may be called from SoftOnly, as an optimization.

    songExportResult.addFlag(PlayerConfigurationFlag::noSoftNoHard);

    const auto volume = cell.getVolume();
    // Optimization: no volume? Then don't encode the noise.
    // BUT this could have some influence on rare occasions (noise on channel 1, noise on channel 3 with volume 0: channel 1 should be heard with the noise of channel 3).
    const auto noise = (volume == 0) ? 0 : cell.getNoise();
    const auto isNoise = (noise > 0);

    BitNumber number;
    number.injectBits("00");                        // Type: 00.
    number.injectBool(false);                       // Not end of sound.
    number.injectNumber(volume, 4U);                // The volume.
    number.injectBool(isNoise);

    sourceGenerator.declareByte(number.get(), "Volume: " + juce::String(volume) + ".");

    // Noise? Encodes it.
    if (isNoise) {
        songExportResult.addFlag(PlayerConfigurationFlag::noSoftNoHardNoise);
        sourceGenerator.declareByte(noise, "Noise.");
    }
}

void AkmInstrumentEncoder::encodeInstrumentCellSoftwareOnly(const LowLevelPsgInstrumentCell& cell) noexcept
{
    jassert(cell.getLink() == PsgInstrumentCellLink::softOnly);

    const auto volume = cell.getVolume();
    // No volume? Optimization: encode as "no software no hardware").
    if (volume == 0) {
        return encodeInstrumentCellNoSoftNoHard(cell);
    }
    songExportResult.addFlag(PlayerConfigurationFlag::softwareOnly);

    auto arpeggio = cell.getSoftwareArpeggio();
    checkArpeggioLimitAndCorrect(arpeggio);
    auto pitch = -cell.getSoftwarePitch();           // Note the sign! A pitch in AT is "sound goes up". But on CPC, the period must go down for the sound to go up.
    checkPitchLimitAndCorrect(pitch);
    checkSoftwarePeriod(cell.getSoftwarePeriod());

    const auto noise = cell.getNoise();
    const auto isNoise = (noise > 0);
    const auto isArpeggio = (arpeggio != 0);
    const auto isPitch = (pitch != 0);
    const auto isNoiseAndOrArpeggio = (isNoise || isArpeggio);

    // First byte.
    {
        BitNumber number;
        number.injectBits("01");                        // Type: 01.
        number.injectNumber(volume, 4U);
        number.injectBool(isPitch);
        number.injectBool(isNoiseAndOrArpeggio);

        sourceGenerator.declareByte(number.get(), "Volume: " + juce::String(volume) + ".");
    }

    // Noise/Arpeggio?
    if (isNoiseAndOrArpeggio) {
        songExportResult.addFlag(PlayerConfigurationFlag::softwareOnlySoftwareArpeggio);

        BitNumber number;
        number.injectBool(isNoise);
        number.injectNumber(arpeggio, 7U);
        sourceGenerator.declareByte(number.get(), "Arpeggio: " + juce::String(arpeggio) + ".");

        // Encodes the noise?
        if (isNoise) {
            songExportResult.addFlag(PlayerConfigurationFlag::softwareOnlyNoise);

            BitNumber numberNoise;
            numberNoise.injectNumber(noise, 8U);
            sourceGenerator.declareByte(numberNoise.get(), "Noise: " + juce::String(noise) + ".");
        }
    }

    // Pitch?
    if (isPitch) {
        songExportResult.addFlag(PlayerConfigurationFlag::softwareOnlySoftwarePitch);

        sourceGenerator.declareWord(pitch, "Pitch: " + juce::String(pitch) + ".");
    }
}

void AkmInstrumentEncoder::encodeInstrumentCellSoftwareToHardware(const LowLevelPsgInstrumentCell& cell) noexcept
{
    jassert(cell.getLink() == PsgInstrumentCellLink::softToHard);
    songExportResult.addFlag(PlayerConfigurationFlag::softwareToHardware);

    // Shared code with the "software and hardware".
    encodeSharedInstrumentCellSoftwareAndToHardware(cell, true);
}

void AkmInstrumentEncoder::encodeInstrumentCellSoftwareAndHardware(const LowLevelPsgInstrumentCell& cell) noexcept
{
    jassert(cell.getLink() == PsgInstrumentCellLink::softAndHard);
    songExportResult.addFlag(PlayerConfigurationFlag::softwareAndHardware);
    songExportResult.addFlag(PlayerConfigurationFlag::softwareAndHardwareForcedHardwarePeriod);     // Only option available ("Ben Daglish" effect).

    // The hardware period should be >0, else this effect is useless.
    auto hardwarePeriod = cell.getHardwarePeriod();
    if (hardwarePeriod == 0) {
        songExportResult.addWarning("The hardware period must be forced to a value, in this format.");
        hardwarePeriod = 1;
    }

    // Shared code with the "software to hardware".
    encodeSharedInstrumentCellSoftwareAndToHardware(cell, false);

    // Encodes the hardware period.
    sourceGenerator.declareWord(hardwarePeriod, "Hardware period: " + juce::String(hardwarePeriod) + ".");
}

void AkmInstrumentEncoder::encodeSharedInstrumentCellSoftwareAndToHardware(const LowLevelPsgInstrumentCell& cell, bool encodeRatio) noexcept
{
    jassert((cell.getLink() == PsgInstrumentCellLink::softToHard) || (cell.getLink() == PsgInstrumentCellLink::softAndHard));
    const auto isSoftToHard = (cell.getLink() == PsgInstrumentCellLink::softToHard);
    const auto linkCode = (cell.getLink() == PsgInstrumentCellLink::softToHard) ? juce::String("10") : "11";

    // Retrig? There shouldn't.
    if (cell.isRetrig()) {
        songExportResult.addWarning("Retrig is not supported in this format.");
    }
    // Noise? There shouldn't.
    if (cell.getNoise() != 0) {
        songExportResult.addWarning("Noise is not supported with a hardware sound.");
    }

    // Checks the Hardware envelope.
    auto hardwareEnvelope = cell.getHardwareEnvelope();
    if ((hardwareEnvelope != 8) && (hardwareEnvelope != 0xa)) {
        songExportResult.addWarning("Only the hardware envelope 8 and 10 are supported in this format.");
        // Forces to 8, unless the invert of A is used.
        hardwareEnvelope = (hardwareEnvelope == 0xe) ? 0xa : 8;
    }

    auto arpeggio = cell.getSoftwareArpeggio();
    checkArpeggioLimitAndCorrect(arpeggio);
    auto pitch = -cell.getSoftwarePitch();           // Note the sign! A pitch in AT is "sound goes up". But on CPC, the period must go down for the sound to go up.
    checkPitchLimitAndCorrect(pitch);
    checkSoftwarePeriod(cell.getSoftwarePeriod());

    const auto isArpeggio = (arpeggio != 0);
    const auto isPitch = (pitch != 0);
    const auto ratio = cell.getRatio();

    // First byte.
    {
        BitNumber number;
        number.injectBits(linkCode);                                        // Type: 10 (soft to hard) or 11 (soft and hard).
        number.injectBool(isPitch);
        number.injectBool(hardwareEnvelope == 0xa);                         // False for 8, true for 0xa.
        jassert((ratio >= 0) && (ratio <= 7));
        int ratioToEncode;    // NOLINT(*-init-variables)
        if (encodeRatio) {
            ratioToEncode = encodeInvertedRatio ? (7 - ratio) : ratio;
        } else {
            ratioToEncode = 0;
        }
        number.injectNumber(ratioToEncode, 3U);             // Inverted ratio, if wanted.
        number.injectBool(isArpeggio);

        sourceGenerator.declareByte(number.get());
    }

    // Arpeggio?
    if (isArpeggio) {
        songExportResult.addFlag(isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareSoftwareArpeggio : PlayerConfigurationFlag::softwareAndHardwareSoftwareArpeggio);

        sourceGenerator.declareByte(arpeggio, "Arpeggio: " + juce::String(arpeggio) + ".");
    }

    // Pitch?
    if (isPitch) {
        songExportResult.addFlag(isSoftToHard ? PlayerConfigurationFlag::softwareToHardwareSoftwarePitch : PlayerConfigurationFlag::softwareAndHardwareSoftwarePitch);

        sourceGenerator.declareWord(pitch, "Pitch: " + juce::String(pitch) + ".");
    }
}

juce::String AkmInstrumentEncoder::getInstrumentLabelWithLoop(int instrumentIndex) noexcept
{
    return getInstrumentLabel(instrumentIndex) + "Loop";
}

void AkmInstrumentEncoder::checkArpeggioLimitAndCorrect(int& arpeggio) noexcept
{
    // Checks the limits.
    if ((arpeggio > maximumArpeggioValueInInstrument) || (arpeggio < minimumArpeggioValueInInstrument)) {
        songExportResult.addWarning("Arpeggio value must be between "
                                                         + juce::String(minimumArpeggioValueInInstrument) + " and "
                                                         + juce::String(maximumArpeggioValueInInstrument) + ".");
        arpeggio = 0;
    }
}

void AkmInstrumentEncoder::checkPitchLimitAndCorrect(int& pitch) noexcept
{
    // Checks the limits.
    if ((pitch > maximumPitchValueInInstrument) || (pitch < minimumPitchValueInInstrument)) {
        songExportResult.addWarning("Pitch value must be between "
                                                         + juce::String(minimumPitchValueInInstrument) + " and "
                                                         + juce::String(maximumPitchValueInInstrument) + ".");
        pitch = 0;
    }
}

void AkmInstrumentEncoder::checkSoftwarePeriod(int softwarePeriod) noexcept
{
    // Must be "auto".
    if (softwarePeriod != 0) {
        songExportResult.addWarning("The software period must be 'auto' (0). No forced software period is authorized.");
    }
}

void AkmInstrumentEncoder::encodeInstrumentEnd(int instrumentIndex, bool loop, int loopIndex) noexcept
{
    BitNumber number;
    number.injectBits("00");                                            // Type: 00 : no soft, no hard.
    number.injectBool(true);
    sourceGenerator.declareByte(number.get(), "End the instrument.");

    sourceGenerator.declarePointerRegionStart();
    if (!loop) {
        // Loops to the sound 0 if there is no loop (=silence).
        sourceGenerator.declareWord(getInstrumentLabelWithLoop(0), "Loop to silence.");
    } else {
        songExportResult.addFlag(PlayerConfigurationFlag::instrumentLoopTo);

        // Loops to a specific index of an Instrument.
        jassert((loopIndex >= 0) && (loopIndex <= 255)); (void)loopIndex;           // To avoid a warning in Release.
        sourceGenerator.declareWord(getInstrumentLabelWithLoop(instrumentIndex), "Loops.");
    }

    sourceGenerator.declarePointerRegionEnd();
}


}   // namespace arkostracker

