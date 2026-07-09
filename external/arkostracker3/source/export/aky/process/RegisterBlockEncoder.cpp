#include "RegisterBlockEncoder.h"

#include "../../../business/sid/StSidPeriodCalculator.h"
#include "../../../utils/BitNumber.h"
#include "../../../utils/NumberUtil.h"
#include "../../sourceGenerator/SourceGenerator.h"
#include "AkyConstants.h"
#include "EncodedLine.h"
#include "RegisterBlock.h"
#include "RegisterBlockLabelProvider.h"
#include "RegisterBlockPool.h"

namespace arkostracker 
{

const juce::String RegisterBlockEncoder::loopLabelSuffix = "_Loop";                 // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

RegisterBlockEncoder::RegisterBlockEncoder(SourceGenerator& pSourceGenerator, RegisterBlockPool& pRegisterBlockPool, RegisterBlockLabelProvider& pRegisterBlockLabelProvider,
                                           const bool pLittleEndian, const SidPlayerCapability& pSidPlayerCapability) noexcept :
    sourceGenerator(pSourceGenerator),
    registerBlockPool(pRegisterBlockPool),
    registerBlockLabelProvider(pRegisterBlockLabelProvider),
    littleEndian(pLittleEndian),
    currentVolume(-1),
    currentNoise(-1),
    //currentSound(-1),
    currentSoftwarePeriod(-1),
    currentHardwarePeriod(-1),
    currentHardwareCurve(-1),
    currentSpecialEffect(),
    currentSidHighShelfDuration(-1),
    currentSidLowShelfDuration(-1),
    currentSidLowShelfVolume(-1),
    sidPlayerCapability(pSidPlayerCapability)
{
}

void RegisterBlockEncoder::clearCurrentRegisters()
{
    currentVolume = -1;
    currentNoise = -1;
    //currentSound = -1;
    currentSoftwarePeriod = -1;
    currentHardwarePeriod = -1;
    currentHardwareCurve = -1;
    currentSpecialEffect = SpecialEffect();
    currentSidHighShelfDuration = -1;
    currentSidLowShelfDuration = -1;
    currentSidLowShelfVolume = -1;
}

void RegisterBlockEncoder::encode(RegisterBlock& registerBlock, const juce::String& label, const juce::String& relativeToLabel) noexcept
{
    clearCurrentRegisters();

    const auto loopLabel = label + loopLabelSuffix;

    // The line to encode also depends on the loop! Potentially, fewer lines to encode!
    const auto loop = registerBlock.isLooping();
    const auto lineCount = loop ? registerBlock.getSizeWithLoop() : registerBlock.getSize();
    jassert(lineCount > 0);

    // Is there a loop? If not, use a fake loop index so that it is never reached.
    const auto loopIndex = loop ? registerBlock.getLoopStartIndex() : -1;

    // There can never be a loop at the initial state!
    jassert(loopIndex != 0);

    // Encodes the initial line first.
    encodeInitialLine(registerBlock, registerBlock.getChannelRegisters(0));

    // Then, all the other lines.
    for (auto lineIndex = 1; lineIndex < lineCount; ++lineIndex) {
        encodeNonInitialLine(registerBlock, registerBlock.getChannelRegisters(lineIndex));
    }

    // Finally, encodes the loop, if any.
    if (loop) {
        // The label may be relative, or not.
        const auto loopLabelMaybeRelative = SourceGenerator::buildRelativeExpression(loopLabel, relativeToLabel);
        // The line count is used which is not really normal, but there should be no problem.
        const auto sequence = EncodedSequence::buildLoopLine(loopLabelMaybeRelative, loopIndex, "Loop to index " + juce::String(loopIndex) + ".");
        const EncodedLine encodedLine(sequence);
        registerBlock.addEncodedLine(encodedLine);
    }
}

void RegisterBlockEncoder::generateSource(const RegisterBlock& registerBlock, const juce::String& label, const juce::String& baseLabel) const noexcept
{
    // Disark: a byte area has been declared by the caller (cleaner source).
    sourceGenerator.declareLabel(label);

    auto loopIndex = registerBlock.getLoopStartIndex();
    const auto loopLabel = label + loopLabelSuffix;

    const auto referenceLoopData = registerBlock.getReferredFromAndReferredToRegisterBlockIdAndIndex();
    const auto referredFromIndex = std::get<0>(referenceLoopData);             // -1 if not useful.
    const auto destinationRegisterBlockId = std::get<1>(referenceLoopData);    // -1 if not useful.
    const auto referredToIndex = std::get<2>(referenceLoopData);               // -1 if not useful.

    // Encodes each line as source. The loop also has to be encoded.
    auto lineIndex = 0;
    for (const auto& encodedLine : registerBlock.getEncodedLines()) {
        // Is there a possible Goto? If yes, encode it and leave, the other data can be optimized away!
        if (lineIndex == referredFromIndex) {
            sourceGenerator.declareByte(AkyConstants::loopByte);

            // Finds the destination RegisterBlock in order to know the offset, in bytes, of the line to go to.
            jassert(destinationRegisterBlockId >= 0);
            const RegisterBlock* destinationRegisterBlock = registerBlockPool.findUsedRegisterBlockFromId(destinationRegisterBlockId);
            jassert(destinationRegisterBlock != nullptr);       // Shouldn't happen.
            if (destinationRegisterBlock == nullptr) {          // Only to avoid a warning.
                break;
            }

            // Instead of generating labels, we simply find the offset in bytes of the line to reach to make a goto to that.
            const auto offsetInBytes = destinationRegisterBlock->getOffsetInBytesOfLineIndex(referredToIndex);
            const auto gotoLabel = registerBlockLabelProvider.getLabelForRegisterBlockIndex(destinationRegisterBlockId, -1) + " + " + juce::String(offsetInBytes);
            // Declares the Goto Label, relative if a base label is given.
            sourceGenerator.declarePointerRegionStart();
            sourceGenerator.declareComment("Optimization: goto common Block at index " + juce::String(referredToIndex) + ".");
            sourceGenerator.declareAddressOrRelativeAddress(gotoLabel, baseLabel);
            sourceGenerator.declarePointerRegionEnd();
            break;
        }

        // Encodes the possible loop label, if we are on the right line.
        if (lineIndex == loopIndex) {
            sourceGenerator.declareLabel(loopLabel);
            loopIndex = -1;     // To prevent double-declaration, possible with "SID" metadata lines.
        }

        encodedLine.generateSource(sourceGenerator);

        ++lineIndex;
    }
}

void RegisterBlockEncoder::encodeInitialLine(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    // Encodes the SID, if any.
    encodeInitialLineSidIfNeeded(registerBlock, channelRegisters);

    switch (channelRegisters.getSoundType()) {
    case SoundType::noSoftwareNoHardware:
        encodeInitialLineNoSoftwareNoHardware(registerBlock, channelRegisters);
        break;
    case SoundType::softwareOnly:
        encodeInitialLineSoftwareOnly(registerBlock, channelRegisters);
        break;
    case SoundType::hardwareOnly:
        encodeInitialLineHardwareOnly(registerBlock, channelRegisters);
        break;
    case SoundType::softwareAndHardware:
        encodeInitialLineSoftwareAndHardware(registerBlock, channelRegisters);
        break;
    default:
        jassertfalse;           // Shouldn't happen.
    }
}

void RegisterBlockEncoder::encodeInitialLineSidIfNeeded(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    const auto& specialEffect = channelRegisters.getSpecialEffect();
    currentSpecialEffect = specialEffect;

    if (!specialEffect.isSidActivated()) {
        return;
    }

    BitNumber bitNumber;
    bitNumber.injectBits("00");
    bitNumber.injectNumber(specialEffect.getLowShelfVolume(), 4);
    bitNumber.injectBits("10");
    EncodedLine sidEncodedLine(EncodedSequence::buildOneByte(bitNumber.get(), "Initial State: SID."));

    encodeSidPeriodsAndStoreThem(registerBlock, channelRegisters, sidEncodedLine);
}

void RegisterBlockEncoder::encodeSidPeriodsAndStoreThem(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters, EncodedLine& sidEncodedLine) noexcept
{
    const auto specialEffect = channelRegisters.getSpecialEffect();
    const auto highShelfPeriod = specialEffect.getHighShelfPeriod();
    const auto lowShelfPeriod = specialEffect.getLowShelfPeriod();

    switch (sidPlayerCapability.getSidPlayerProfile()) {
        case SidPlayerProfile::cpc:
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(highShelfPeriod, "8-bit SID high-shelf duration."));
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(lowShelfPeriod, "8-bit SID low-shelf duration."));
            break;
        case SidPlayerProfile::amstradPlus:
        case SidPlayerProfile::custom:
            sidEncodedLine.addSequence(EncodedSequence::buildWord(highShelfPeriod, "16-bit SID high-shelf duration.", littleEndian));
            sidEncodedLine.addSequence(EncodedSequence::buildWord(lowShelfPeriod, "16-bit SID low-shelf duration.", littleEndian));
            break;
        case SidPlayerProfile::atariSt: {
            // Atari ST has a special encoding.
            const auto [highShelfTimerControl, highShelfTimerData] = StSidPeriodCalculator::calculate(highShelfPeriod);
            const auto [lowShelfTimerControl, lowShelfTimerData] = StSidPeriodCalculator::calculate(lowShelfPeriod);
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(highShelfTimerControl, "SID high-shelf timer control."));
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(highShelfTimerData, "SID high-shelf timer data."));
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(lowShelfTimerControl, "SID low-shelf timer control."));
            sidEncodedLine.addSequence(EncodedSequence::buildOneByte(lowShelfTimerData, "SID low-shelf timer data."));
            break;
        }
    }

    registerBlock.addPendingEncodedLine(sidEncodedLine);        // This line will be merged into the next one.

    currentSidHighShelfDuration = highShelfPeriod;
    currentSidLowShelfDuration = lowShelfPeriod;
    currentSidLowShelfVolume = specialEffect.getLowShelfVolume();
}

void RegisterBlockEncoder::encodeInitialLineNoSoftwareNoHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    const auto newNoise = channelRegisters.getNoise();
    const auto newVolume = channelRegisters.getVolume();

    const auto isNoise = (newNoise > 0);
    auto byteToEncode = 0b00 | (newVolume << 3);     // b3-b6: volume.

    // Noise?
    if (isNoise) {
        byteToEncode |= (1 << 2);           // b2: noise?
    }

    EncodedLine encodedLine(EncodedSequence::buildOneByte(byteToEncode, "Initial State: no software, no hardware."));

    // Encodes the noise, if any.
    if (isNoise) {
        currentNoise = newNoise;

        encodedLine.addSequence(EncodedSequence::buildOneByte(newNoise, "Noise."));

    } else {
        currentNoise = 0;
    }

    registerBlock.addEncodedLine(encodedLine);

    //currentSound = 0;
}

void RegisterBlockEncoder::encodeInitialLineSoftwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    currentNoise = channelRegisters.getNoise();
    const auto isNoise = (currentNoise > 0);
    currentVolume = channelRegisters.getVolume();
    currentSoftwarePeriod = channelRegisters.getSoftwarePeriod();
    //currentSound = 1;

    auto byteToEncode = 0b01 | (currentVolume << 3);                 // b3-b6: volume.
    byteToEncode |= (NumberUtil::boolToInt(isNoise) << 2);          // b2 = noise?

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(byteToEncode, "Initial State: software only."));

    // Encodes the noise, if any.
    if (isNoise) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(currentNoise, "Noise."));
    }

    // Encodes the software period.
    encodedLine.addSequence(EncodedSequence::buildWord(currentSoftwarePeriod, "Software period.", littleEndian));

    registerBlock.addEncodedLine(encodedLine);
}

void RegisterBlockEncoder::encodeInitialLineHardwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    currentNoise = channelRegisters.getNoise();
    currentVolume = channelRegisters.getVolume();
    currentHardwarePeriod = channelRegisters.getHardwarePeriod();
    currentHardwareCurve = channelRegisters.getHardwareEnvelope();
    //currentSound = 0;       // No sound, hardware only.

    const auto isNoise = (currentNoise > 0);

    auto firstByte = 0b10 | (NumberUtil::boolToInt(channelRegisters.isRetrig()) << 2);       // b2: retrig?
    firstByte |= (NumberUtil::boolToInt(isNoise) << 3);                                     // b3: noise?
    firstByte |= (currentHardwareCurve << 4);                                               // b4-7: envelope.

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, "Initial State: hardware only."));

    // Noise?
    if (isNoise) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(currentNoise, "Noise."));
    }

    // The hardware period.
    encodedLine.addSequence(EncodedSequence::buildWord(currentHardwarePeriod, "Hardware period.", littleEndian));

    registerBlock.addEncodedLine(encodedLine);
}

void RegisterBlockEncoder::encodeInitialLineSoftwareAndHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    currentNoise = channelRegisters.getNoise();
    currentVolume = channelRegisters.getVolume();
    currentSoftwarePeriod = channelRegisters.getSoftwarePeriod();
    currentHardwarePeriod = channelRegisters.getHardwarePeriod();
    currentHardwareCurve = channelRegisters.getHardwareEnvelope();
    //currentSound = 1;       // Sound to on.
    const auto isNoise = (currentNoise > 0);

    const auto retrig = channelRegisters.isRetrig();
    auto firstByte = 0b11 | (NumberUtil::boolToInt(retrig) << 2);                            // b2: retrig?
    firstByte |= (NumberUtil::boolToInt(isNoise) << 3);                                     // b3: noise?
    firstByte |= (currentHardwareCurve << 4);                                               // b4-7: envelope.

    juce::String comment("Initial State : software and hardware.");
    if (retrig) {
        comment += " Retrig.";
    }

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, comment));

    // Noise, if present.
    if (isNoise) {
        sourceGenerator.declareByte(currentNoise, "Noise.");
        encodedLine.addSequence(EncodedSequence::buildOneByte(currentNoise, "Noise."));
    }

    // The periods.
    encodedLine.addSequence(EncodedSequence::buildWord(currentSoftwarePeriod, "Software period.", littleEndian));
    encodedLine.addSequence(EncodedSequence::buildWord(currentHardwarePeriod, "Hardware period.", littleEndian));

    registerBlock.addEncodedLine(encodedLine);
}


// Non-initial lines.
// ====================================================================

void RegisterBlockEncoder::encodeNonInitialLine(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    encodeNonInitialLineSidIfNeeded(registerBlock, channelRegisters);

    // Encodes the data of this line.
    switch (channelRegisters.getSoundType()) {
        case SoundType::noSoftwareNoHardware:
            encodeNonInitialLineNoSoftwareNoHardware(registerBlock, channelRegisters);
            break;
        case SoundType::softwareOnly:
            encodeNonInitialLineSoftwareOnly(registerBlock, channelRegisters);
            break;
        case SoundType::hardwareOnly:
            encodeNonInitialLineHardwareOnly(registerBlock, channelRegisters);
            break;
        case SoundType::softwareAndHardware:
            encodeNonInitialLineSoftwareAndHardware(registerBlock, channelRegisters);
            break;
        default:
            jassertfalse; // Shouldn't happen.
    }
}

void RegisterBlockEncoder::encodeNonInitialLineSidIfNeeded(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    const auto& specialEffect = channelRegisters.getSpecialEffect();
    const auto isPreviousSidActivated = currentSpecialEffect.isSidActivated();
    // No SID effect before, no SID effect now? Then don't do anything.
    if (!isPreviousSidActivated && !specialEffect.isSidActivated()) {
        return;
    }
    // This might be an SID OFF, or a SID ON (period will be encoded), or a SID continuation (SID flag and period encoded only if different).
    currentSpecialEffect = specialEffect;

    const auto isSidActivated = specialEffect.isSidActivated();
    if (!isSidActivated) {
        BitNumber bitNumber;
        bitNumber.injectBool(isSidActivated);

        EncodedLine sidEncodedLine(EncodedSequence::buildOneByte(AkyConstants::sidNisByte, "Non-initial State: SID."));
        sidEncodedLine.addSequence(EncodedSequence::buildOneByte(bitNumber.get(), "SID new state: OFF."));
        registerBlock.addPendingEncodedLine(sidEncodedLine);
        return;
    }

    // SID is activated. But it must be encoded only if the periods are different, or if SID was off and is now on.
    const auto sidLowShelfVolume = specialEffect.getLowShelfVolume();
    if (!isPreviousSidActivated) {
        currentSidHighShelfDuration = -1;       // Hack... Forces the encoding in case the SID was off.
        currentSidLowShelfDuration = -1;
        currentSidLowShelfVolume = -1;
    }

    if ((specialEffect.getHighShelfPeriod() == currentSidHighShelfDuration)
        && (specialEffect.getLowShelfPeriod() == currentSidLowShelfDuration)
        && (sidLowShelfVolume == currentSidLowShelfVolume)
    ) {
        return;
    }

    BitNumber bitNumber;
    bitNumber.injectBool(isSidActivated);
    bitNumber.injectNumber(sidLowShelfVolume, 4);

    EncodedLine sidEncodedLine(EncodedSequence::buildOneByte(AkyConstants::sidNisByte, "Non-initial State: SID."));
    sidEncodedLine.addSequence(EncodedSequence::buildOneByte(bitNumber.get(), "SID state + low-shelf volume."));

    // Encodes the periods.
    encodeSidPeriodsAndStoreThem(registerBlock, channelRegisters, sidEncodedLine);
}

void RegisterBlockEncoder::encodeNonInitialLineNoSoftwareNoHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) const noexcept
{
    // Note: the loop is not encoded here, but as special case, independently.

    const auto newVolume = channelRegisters.getVolume();
    const auto newNoise = channelRegisters.getNoise();
    const auto isNewVolume = (newVolume != currentVolume);
    const auto isNoise = (newNoise > 0);
    //bool isNewNoise = (newNoise != currentNoise);

    //bool encodeNoise = (isNoise && isNewNoise);       // If no noise, no need to encode the noise, even if it has changed!

    auto firstByte = (0b00 | (isNewVolume << 2) | (NumberUtil::boolToInt(isNoise) << 7));
    if (isNewVolume) {
        firstByte |= (newVolume << 3);
    }
    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, "Non-initial State, no software no hardware."));

    // If noise, a new byte must be allocated.
    if (isNoise) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(newNoise, "Noise."));
    }

    registerBlock.addEncodedLine(encodedLine);
}

void RegisterBlockEncoder::encodeNonInitialLineSoftwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    auto firstByte = 0b01 | (channelRegisters.getVolume() << 2);

    // New MSB and/or noise?
    const auto newNoise = channelRegisters.getNoise();
    const auto isNoise = (newNoise > 0);
    const auto isNewNoise = (newNoise != currentNoise);

    // If the current Software period is undefined, it must be forced because we need it!
    const auto newSoftwarePeriod = channelRegisters.getSoftwarePeriod() & 0xfff;   // TODO for future versions: there should never be period > 0xfff (yet this appears in SlapFight3 song, third pattern with shift).
    const auto lsbPeriod = newSoftwarePeriod & 0xff;
    const auto msbPeriod = (newSoftwarePeriod / 256);     // No mask, done just below.

    const auto isNewLsbPeriod = (currentSoftwarePeriod < 0) || (lsbPeriod != (currentSoftwarePeriod & 0xff));
    const auto isNewMsbPeriod = (currentSoftwarePeriod < 0) || (msbPeriod != ((currentSoftwarePeriod / 256) & 0xf));
    const auto isNewNoiseOrNewMsbPeriod = (isNoise || isNewMsbPeriod);

    // Enough data to encode the first byte.
    firstByte |= (NumberUtil::boolToInt(isNewLsbPeriod) << 6);
    firstByte |= (NumberUtil::boolToInt(isNewNoiseOrNewMsbPeriod) << 7);

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, "Non-initial State, software only."));

    // Encodes the LSB, if needed.
    if (isNewLsbPeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(lsbPeriod, "New LSB for software period."));
    }
    // Encodes the MSB and/or noise.
    if (isNewNoiseOrNewMsbPeriod) {
        const auto encodeNewNoise = isNoise && isNewNoise;
        const auto byteToEncode = msbPeriod
                                 | (NumberUtil::boolToInt(encodeNewNoise) << 6)      // There must be a noise else there is no need to encode the noise!
                                 | (NumberUtil::boolToInt(isNoise) << 7);
        encodedLine.addSequence(EncodedSequence::buildOneByte(byteToEncode, "New MSB for software period, maybe with noise."));

        if (encodeNewNoise) {
            encodedLine.addSequence(EncodedSequence::buildOneByte(newNoise, "New noise."));
            currentNoise = newNoise;
        }
    }

    currentSoftwarePeriod = newSoftwarePeriod;

    registerBlock.addEncodedLine(encodedLine);
}

void RegisterBlockEncoder::encodeNonInitialLineHardwareOnly(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    const auto retrig = channelRegisters.isRetrig();
    const auto newNoise = channelRegisters.getNoise();
    const auto isNoise = (newNoise > 0);
    const auto isNewNoise = isNoise && (newNoise != currentNoise);            // No new noise if there is no noise!
    const auto isNoiseOrRetrig = retrig || isNoise || isNewNoise;
    currentHardwareCurve = channelRegisters.getHardwareEnvelope();

    // If the hardware period is undefined, it must be forced!
    const auto newHardwarePeriod = channelRegisters.getHardwarePeriod();
    const auto isNewLsbPeriod = (currentHardwarePeriod < 0) || ((newHardwarePeriod & 0xff) != (currentHardwarePeriod & 0xff));
    const auto isNewMsbPeriod = (currentHardwarePeriod < 0) || (((newHardwarePeriod / 256) & 0xff) != ((currentHardwarePeriod / 256) & 0xff));

    currentHardwarePeriod = newHardwarePeriod;
    if (isNewNoise) {
        currentNoise = newNoise;
    }

    // Enough data to encode the first byte.
    auto firstByte = 0b10;
    // b2-b4: hardware envelope (3 bits only, losing bit 4 which is always 1). It will be added by the player.
    firstByte |= ((currentHardwareCurve & 0b111) << 2);
    firstByte |= (NumberUtil::boolToInt(isNoiseOrRetrig) << 5);                  // b5: noise and/or retrig?
    firstByte |= (NumberUtil::boolToInt(isNewMsbPeriod) << 6);
    firstByte |= (NumberUtil::boolToInt(isNewLsbPeriod) << 7);

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, "Non-initial State, hardware only."));

    // LSP/MSP.
    if (isNewLsbPeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte((currentHardwarePeriod & 0xff), "Hardware period, LSB"));
    }
    if (isNewMsbPeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(((currentHardwarePeriod / 256) & 0xff), "Hardware period, MSB"));
    }

    // Noise/retrig?
    encodeNoiseOrRetrigIfNeeded(encodedLine, isNoiseOrRetrig, retrig, isNoise, isNewNoise, newNoise);

    registerBlock.addEncodedLine(encodedLine);
}

void RegisterBlockEncoder::encodeNoiseOrRetrigIfNeeded(EncodedLine& encodedLine, const bool isNoiseOrRetrig, const bool retrig, const bool isNoise, const bool isNewNoise, const int noiseValue) noexcept
{
    if (isNoiseOrRetrig) {
        const auto byteToEncode = (NumberUtil::boolToInt(retrig) | (NumberUtil::boolToInt(isNoise) << 1)
                                  | ((NumberUtil::boolToInt(isNoise && isNewNoise) << 2))      // If there is no noise, no need to encode the isNewNoise.
                                  | (noiseValue << 3));

        encodedLine.addSequence(EncodedSequence::buildOneByte(byteToEncode, "Noise and/or retrig."));
    }
}

void RegisterBlockEncoder::encodeNonInitialLineSoftwareAndHardware(RegisterBlock& registerBlock, const ChannelOutputRegisters& channelRegisters) noexcept
{
    const auto retrig = channelRegisters.isRetrig();
    const auto newNoise = channelRegisters.getNoise();
    const auto isNoise = (newNoise > 0);
    const auto isNewNoise = isNoise && (newNoise != currentNoise);        // No new noise if there is no noise!
    const auto isNoiseOrRetrig = retrig || isNoise || isNewNoise;
    const auto newHardwareEnvelope = channelRegisters.getHardwareEnvelope();
    const auto isNewHardwareEnvelope = (newHardwareEnvelope != currentHardwareCurve);
    currentHardwareCurve = newHardwareEnvelope;

    // If the hardware period is undefined, it must be forced!
    const auto newHardwarePeriod = channelRegisters.getHardwarePeriod();
    const auto isNewLsbHardwarePeriod = (currentHardwarePeriod < 0) || ((newHardwarePeriod & 0xff) != (currentHardwarePeriod & 0xff));
    const auto isNewMsbHardwarePeriod = (currentHardwarePeriod < 0) || (((newHardwarePeriod / 256) & 0xff) != ((currentHardwarePeriod / 256) & 0xff));
    currentHardwarePeriod = newHardwarePeriod;

    // The same for the software period.
    const auto newSoftwarePeriod = channelRegisters.getSoftwarePeriod();
    const auto isNewLsbSoftwarePeriod = (currentSoftwarePeriod < 0) || ((newSoftwarePeriod & 0xff) != (currentSoftwarePeriod & 0xff));
    const auto isNewMsbSoftwarePeriod = (currentSoftwarePeriod < 0) || (((newSoftwarePeriod / 256) & 0xff) != ((currentSoftwarePeriod / 256) & 0xff));
    currentSoftwarePeriod = newSoftwarePeriod;

    if (isNewNoise) {
        currentNoise = newNoise;
    }

    // Enough data to encode the first byte.
    auto firstByte = 0b11;
    firstByte |= (NumberUtil::boolToInt(isNewLsbHardwarePeriod) << 2);
    firstByte |= (NumberUtil::boolToInt(isNewMsbHardwarePeriod) << 3);
    firstByte |= (NumberUtil::boolToInt(isNewLsbSoftwarePeriod) << 4);
    firstByte |= (NumberUtil::boolToInt(isNewMsbSoftwarePeriod) << 5);
    firstByte |= (NumberUtil::boolToInt(isNewHardwareEnvelope) << 6);
    firstByte |= (NumberUtil::boolToInt(isNoiseOrRetrig) << 7);          // b7: noise and/or retrig?

    EncodedLine encodedLine;
    encodedLine.addSequence(EncodedSequence::buildOneByte(firstByte, "Non-initial State, software and hardware."));

    // LSB/MSB for hardware period.
    if (isNewLsbHardwarePeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte((currentHardwarePeriod & 0xff), "Hardware period, LSB."));
    }
    if (isNewMsbHardwarePeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(((currentHardwarePeriod / 256) & 0xff), "Hardware period, MSB."));
    }

    // LSB/MSB for software period.
    if (isNewLsbSoftwarePeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte((currentSoftwarePeriod & 0xff), "Software period, LSB."));
    }
    if (isNewMsbSoftwarePeriod) {
        encodedLine.addSequence(EncodedSequence::buildOneByte((currentSoftwarePeriod / 256) & 0xff, "Software period, MSB."));
    }

    // New envelope?
    if (isNewHardwareEnvelope) {
        encodedLine.addSequence(EncodedSequence::buildOneByte(currentHardwareCurve, "Hardware period."));
    }

    // Noise/retrig?
    encodeNoiseOrRetrigIfNeeded(encodedLine, isNoiseOrRetrig, retrig, isNoise, isNewNoise, newNoise);

    registerBlock.addEncodedLine(encodedLine);
}

}   // namespace arkostracker
