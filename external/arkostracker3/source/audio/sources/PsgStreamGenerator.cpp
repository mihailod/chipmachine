#include "PsgStreamGenerator.h"

#include "../../player/PsgPeriod.h"
#include "../../player/PsgRegistersProvider.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../effect/AudioUtil.h"

namespace arkostracker
{

PsgStreamGenerator::PsgStreamGenerator(PsgRegistersProvider& pPsgRegistersProvider, const PsgType pPsgType, const int pPsgIndex, const float pReplayFrequency,
                                       const int pPsgFrequency, const int pSamplePlayerFrequency, const PsgMixingOutput pPsgMixingOutput,
                                       const double pChannelAMixVolume, const double pChannelBMixVolume, const double pChannelCMixVolume,
                                       const SidPlayerCapability& pSidPlayerCapability) noexcept :
        hardwareTables(),
        newDataFromUi(false),
        dataFromUiThreadMutex(),
        mutedChannelIndexesFromUiThread(),

        psgRegistersProvider(pPsgRegistersProvider),
        psgType(pPsgType),
        psgIndex(pPsgIndex),
        replayFrequency(pReplayFrequency),
        psgFrequency(pPsgFrequency),
        samplePlayerFrequency(pSamplePlayerFrequency),
        psgMixingOutput(pPsgMixingOutput),
        channelAMixVolume(pChannelAMixVolume),
        channelBMixVolume(pChannelBMixVolume),
        channelCMixVolume(pChannelCMixVolume),

        sampleRate(44100.0),

        resamplingRatio(1.0),
        generationBufferSize(0),
        generationBuffers(),
        resampledBuffers(),

        sumToReportPerChannel(),
        ratioToReportPerChannel(),

        playerPeriod(0),
        playerPeriodCounter(0),
        periodCounterA(0),
        periodCounterB(0),
        periodCounterC(0),
        periodA(0),
        periodB(0),
        periodC(0),
        isHighShelfA(false),
        isHighShelfB(false),
        isHighShelfC(false),
        hardwarePeriodCounter(0),
        hardwarePeriod(minimumHardwarePeriod),
        hardwareEnvelope(0),
        hardwareCurveCounter(0),
        noise(),
        noisePeriod(0),
        noisePeriodCounter(0),
        isNoiseHighShelf(true),
        noiseSeed(0x1U),         // Any non-zero value will work.

        isMixerSoundAOpen(false),
        isMixerSoundBOpen(false),
        isMixerSoundCOpen(false),
        isMixerNoiseAOpen(false),
        isMixerNoiseBOpen(false),
        isMixerNoiseCOpen(false),
        isMixerNoiseOnAnyChannel(false),
        volumeAFrom0To31Or32(0),
        volumeBFrom0To31Or32(0),
        volumeCFrom0To31Or32(0),

        signalChannelA(),
        signalChannelB(),
        signalChannelC(),

        mutedChannelIndexes(),

        channelASampleDataInfo(),
        sampleAIndex(0.0),
        sampleAStep(0.0),
        playSampleA(false),
        channelBSampleDataInfo(),
        sampleBIndex(0.0),
        sampleBStep(0.0),
        playSampleB(false),
        channelCSampleDataInfo(),
        sampleCIndex(0.0),
        sampleCStep(0.0),
        playSampleC(false),

        isSidOnChannelA(),
        isSidHighShelfA(true),
        sidLowShelfVolumeA(),
        currentSidPeriodA(),
        periodAHighShelf(),
        periodALowShelf(),
        pendingPeriodAHighShelf(),
        pendingPeriodALowShelf(),

        isSidOnChannelB(),
        isSidHighShelfB(true),
        sidLowShelfVolumeB(),
        currentSidPeriodB(),
        periodBHighShelf(),
        periodBLowShelf(),
        pendingPeriodBHighShelf(),
        pendingPeriodBLowShelf(),

        isSidOnChannelC(),
        isSidHighShelfC(true),
        sidLowShelfVolumeC(),
        currentSidPeriodC(),
        periodCHighShelf(),
        periodCLowShelf(),
        pendingPeriodCHighShelf(),
        pendingPeriodCLowShelf(),

        sidPlayerCapability(pSidPlayerCapability)
{
    jassert((pChannelAMixVolume >= 0.0) && (pChannelAMixVolume <= 1.0));
    jassert((pChannelBMixVolume >= 0.0) && (pChannelBMixVolume <= 1.0));
    jassert((pChannelCMixVolume >= 0.0) && (pChannelCMixVolume <= 1.0));

    // Creates the buffers, as we know we only manage stereo.
    for (size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        generationBuffers[channelIndex] = juce::HeapBlock<double>();    // NOLINT(*-pro-bounds-constant-array-index)
        resampledBuffers[channelIndex] = juce::HeapBlock<double>();     // NOLINT(*-pro-bounds-constant-array-index)
    }
}

void PsgStreamGenerator::setMutedChannelIndexes(const std::unordered_set<int>& newMutedChannelIndexes) noexcept
{
    // Posts the new data, for the Audio thread to pick up.
    const std::scoped_lock lock(dataFromUiThreadMutex);                    // Lock!
    if ((mutedChannelIndexesFromUiThread != nullptr) && (*mutedChannelIndexesFromUiThread == newMutedChannelIndexes)) {
        return;
    }
    mutedChannelIndexesFromUiThread = std::make_unique<std::unordered_set<int>>(newMutedChannelIndexes);    // Makes a copy.

    newDataFromUi = true;
}

bool PsgStreamGenerator::isPsgMetadataEqual(const PsgType pPsgType, const int pPsgFrequency, const float pReplayFrequency,
                                            const int pSamplePlayerFrequency,
                                            const double pChannelAVolume, const double pChannelBVolume, const double pChannelCVolume,
                                            const PsgMixingOutput pPsgMixingOutput, const SidPlayerCapability& pSidPlayerCapability) const noexcept
{
    return ((psgType == pPsgType) && (psgFrequency == pPsgFrequency) && juce::exactlyEqual(replayFrequency, pReplayFrequency) &&
            (samplePlayerFrequency == pSamplePlayerFrequency) &&
            juce::exactlyEqual(channelAMixVolume, pChannelAVolume) && juce::exactlyEqual(channelBMixVolume, pChannelBVolume) &&
            juce::exactlyEqual(channelCMixVolume, pChannelCVolume) && (psgMixingOutput == pPsgMixingOutput) &&
            sidPlayerCapability == pSidPlayerCapability);
}

float PsgStreamGenerator::getReplayFrequencyHz() const noexcept
{
    return replayFrequency;
}


// AudioSource method implementations.
// =============================================

void PsgStreamGenerator::prepareToPlay(const int samplesPerBlockExpected, const double outputSampleRate)
{
    // Called from UI thread.
    DBG("AyStreamGenerator::prepareToPlay for psg index " + juce::String(psgIndex) + ". SampleRate = " + juce::String(outputSampleRate) + ". BufferSize = " + juce::String(samplesPerBlockExpected));

    const auto inputSampleRate = psgFrequency;
    resamplingRatio = static_cast<double>(inputSampleRate) / outputSampleRate;
    const auto newGenerationBufferSize = static_cast<int>((static_cast<double>(samplesPerBlockExpected) * resamplingRatio));
    DBG("           Non-resampled buffer size = " + juce::String(newGenerationBufferSize));

    for (size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        generationBuffers.at(channelIndex).allocate(newGenerationBufferSize, false);
        resampledBuffers.at(channelIndex).allocate(samplesPerBlockExpected, false);

        sumToReportPerChannel.at(channelIndex) = 0.0;
        ratioToReportPerChannel.at(channelIndex) = 0.0;
    }

    generationBufferSize = newGenerationBufferSize;

    // How soon must the registers be fetched?
    sampleRate = outputSampleRate;
    playerPeriod = static_cast<int>(static_cast<float>(inputSampleRate) / replayFrequency);
    playerPeriodCounter = playerPeriod + 1;                     // Forces the getting of registers on first pass.
}

void PsgStreamGenerator::releaseResources()
{
    // Nothing to do.
}

void PsgStreamGenerator::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Called from audio thread.

    // There MUST be 2 channels to fill.
    const auto bufferChannelCount = bufferToFill.buffer->getNumChannels();
    if (bufferChannelCount != channelCount) {
        jassertfalse;
        return;
    }

    // Any new data from the UI?
    if (newDataFromUi) {
        const std::scoped_lock lock(dataFromUiThreadMutex);                    // Lock!
        // New muted states?
        if (mutedChannelIndexesFromUiThread != nullptr) {
            mutedChannelIndexes = *mutedChannelIndexesFromUiThread;
            mutedChannelIndexesFromUiThread.reset();
        }

        newDataFromUi = false;
    }

    // Creates the non-resampled AY signal. This fills the "generation" buffers.
    generateSignal();

    // Where to send the data? Gets the JUCE pointer to the output buffers.
    const auto juceOutputBufferStartPosition = bufferToFill.startSample;
    auto& juceOutputBuffer = *bufferToFill.buffer;
    const auto juceOutputBufferSize = juceOutputBuffer.getNumSamples();

    // Performs the resampling.
    const auto localGenerationBufferSize = generationBufferSize;

    for (size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        auto* juceOutputBufferPtr = juceOutputBuffer.getWritePointer(static_cast<int>(channelIndex), juceOutputBufferStartPosition);

        const auto* localGenerationBufferPtrAdvancing = generationBuffers.at(channelIndex).get();

        auto readCountInGenerationBuffer = 0;
        const auto localResamplingRatio = resamplingRatio;

        auto localRatioToReport = ratioToReportPerChannel.at(channelIndex);
        auto localSumToReport = sumToReportPerChannel.at(channelIndex);

        for (auto outputBufferPosition = 0; outputBufferPosition < juceOutputBufferSize; ++outputBufferPosition) {
            auto decreasingResamplingRatio = localResamplingRatio;
            auto sum = 0.0;

            // Maybe there was an "unfinished" sample?
            if (localRatioToReport > 0.0) {
                sum = localSumToReport * localRatioToReport;
                decreasingResamplingRatio -= localRatioToReport;
                localSumToReport = 0.0;
                localRatioToReport = 0.0;
            }

            // Main part. Take care of the "integer" part. Example: 1000000/44100 = 22.675736961. Take only 22.
            while (decreasingResamplingRatio >= 1.0) {
                jassert (readCountInGenerationBuffer < localGenerationBufferSize);     // If larger, big problem! Overflow!
                sum += *localGenerationBufferPtrAdvancing;
                ++localGenerationBufferPtrAdvancing;
                ++readCountInGenerationBuffer;

                --decreasingResamplingRatio;
            }

            // Finally, there can be a remaining part (0.675736961 in the example above).
            // It is applied to the sum, but the input cursor remains here: the value will ALSO be applied in the next output value (with an inverted ratio).

            if (decreasingResamplingRatio > 0.0) {
                // This is the next sample, used as the last part of this output byte, but also as the first of the next (with an "inverted" ratio).

                // Makes sure there are still values to read. Due to approximation, this can happen.
                if (readCountInGenerationBuffer < localGenerationBufferSize) {
                    const auto readValue = *localGenerationBufferPtrAdvancing;
                    ++localGenerationBufferPtrAdvancing;
                    ++readCountInGenerationBuffer;

                    sum += readValue * decreasingResamplingRatio;
                    // The next value will have the invert of the ratio to compensate the fact it was "cut" in this iteration.
                    localSumToReport = readValue;
                    localRatioToReport = 1.0 - decreasingResamplingRatio;      // In our example, 0.324263039.
                }
            }

            const auto value = sum / localResamplingRatio;

            // Writes the value in the output buffer! Done for this sample!
            juceOutputBufferPtr[outputBufferPosition] = static_cast<float>(value);
        }

        // Security. Did we read all the bytes from the generation buffer?
        jassert(readCountInGenerationBuffer == localGenerationBufferSize);     // If larger, big problem! Overflow!

        // Stores the data to report for the next time.
        ratioToReportPerChannel.at(channelIndex) = localRatioToReport;
        sumToReportPerChannel.at(channelIndex) = localSumToReport;
    }
}


// ================================================================

void PsgStreamGenerator::generateSignal() noexcept // NOLINT(*-function-cognitive-complexity)
{
    const auto localGenerationBufferSize = generationBufferSize;

    const auto localPsgType = psgType;
    const auto& volumeTable16Bits = (localPsgType == PsgType::ay) ? Volumes::baseVolumesAy : Volumes::baseVolumesYm;

    // Generates the square sounds.
    auto generationBufferPosition = 0;
    auto* leftGenerationBufferPtrAdvancing = generationBuffers.at(0).get();     // The buffers to fill.
    auto* rightGenerationBufferPtrAdvancing = generationBuffers.at(1).get();

    // Some set-up to store the signal for listeners. Only a small part of it is stored.
    const auto signalToStoreStepLimit = static_cast<int>((static_cast<double>(localGenerationBufferSize) / static_cast<double>(signalReadBufferSize)) * 2.0);   // A ratio to squeeze more data into a signal!
    auto signalCounter = static_cast<int>(signalToStoreStepLimit);       // Increases, when reaches the limit, the data must be saved. Set to a close value for a first byte to be saved quickly.

    // Any channel muted? Do that outside the loop.
    const auto isChannelAMuted = (mutedChannelIndexes.find(0) != mutedChannelIndexes.cend());
    const auto isChannelBMuted = (mutedChannelIndexes.find(1) != mutedChannelIndexes.cend());
    const auto isChannelCMuted = (mutedChannelIndexes.find(2) != mutedChannelIndexes.cend());

    while (generationBufferPosition < localGenerationBufferSize) {

        // Any need for new registers? Every 50hz, for example.
        if (++playerPeriodCounter >= playerPeriod) {
            //DBG("*** NEED REGISTERS for PSG " + juce::String(psgIndex) + ", playerPeriod = " + juce::String(playerPeriod));
            playerPeriodCounter = 0;
            readNextPsgRegisters();
        }

        // Increases all the periods.
        if (++periodCounterA >= (periodA * periodMultiplier)) {
            isHighShelfA = !isHighShelfA;
            periodCounterA = 0;
        }

        if (++periodCounterB >= (periodB * periodMultiplier)) {
            isHighShelfB = !isHighShelfB;
            periodCounterB = 0;
        }

        if (++periodCounterC >= (periodC * periodMultiplier)) {
            isHighShelfC = !isHighShelfC;
            periodCounterC = 0;
        }

        // Manages the Hardware Envelope on the background. Even if not used, still moving.
        if (++hardwarePeriodCounter >= (hardwarePeriod * periodMultiplier)) {
            hardwarePeriodCounter = 0;
            if (++hardwareCurveCounter > (HardwareTables::hardwareCurvesLength - 1)) {
                hardwareCurveCounter = HardwareTables::hardwareCurvesLoopTo;
            }
        }

        // Manages the SID. It may change the current volume.
        auto localVolumeAFrom0To31Or32 = volumeAFrom0To31Or32;
        auto localVolumeBFrom0To31Or32 = volumeBFrom0To31Or32;
        auto localVolumeCFrom0To31Or32 = volumeCFrom0To31Or32;
        manageSid(localVolumeAFrom0To31Or32, isSidOnChannelA, isSidHighShelfA, sidLowShelfVolumeA, currentSidPeriodA,
            periodAHighShelf, periodALowShelf, pendingPeriodAHighShelf, pendingPeriodALowShelf,
            psgFrequency, sidPlayerCapability);
        manageSid(localVolumeBFrom0To31Or32, isSidOnChannelB, isSidHighShelfB, sidLowShelfVolumeB, currentSidPeriodB,
            periodBHighShelf, periodBLowShelf, pendingPeriodBHighShelf, pendingPeriodBLowShelf,
            psgFrequency, sidPlayerCapability);
        manageSid(localVolumeCFrom0To31Or32, isSidOnChannelC, isSidHighShelfC, sidLowShelfVolumeC, currentSidPeriodC,
            periodCHighShelf, periodCLowShelf, pendingPeriodCHighShelf, pendingPeriodCLowShelf,
            psgFrequency, sidPlayerCapability);


        // Determines the channel A/B/C signal, from 0 to 65535 along with the hardware envelope management.
        auto volumeA16Bits = manageHardwareSound(isHardwareEnvelopeUsedOnA(), isSidOnChannelA, isSidHighShelfA, isMixerSoundAOpen, isHighShelfA,
            hardwareTables, localVolumeAFrom0To31Or32, volumeTable16Bits, hardwareEnvelope, hardwareCurveCounter);
        auto volumeB16Bits = manageHardwareSound(isHardwareEnvelopeUsedOnB(), isSidOnChannelB, isSidHighShelfB, isMixerSoundBOpen, isHighShelfB,
            hardwareTables, localVolumeBFrom0To31Or32, volumeTable16Bits, hardwareEnvelope, hardwareCurveCounter);
        auto volumeC16Bits = manageHardwareSound(isHardwareEnvelopeUsedOnC(), isSidOnChannelC, isSidHighShelfC, isMixerSoundCOpen, isHighShelfC,
            hardwareTables, localVolumeCFrom0To31Or32, volumeTable16Bits, hardwareEnvelope, hardwareCurveCounter);





        // Manages the noise.
        if (++noisePeriodCounter >= (noisePeriod * periodMultiplier)) {
            noisePeriodCounter = 0;

            // Noise generation randomness studied by Zik/Futurs' from the real stuff. He knows what he's talking about.
            // 17bit lfsr. (x^17 + x^3 + 1), and linear span is 17.
            isNoiseHighShelf = ((noiseSeed & 1U) ^ 1U) != 0U;
            const auto bits = ((noiseSeed >> 0U) ^ (noiseSeed >> 3U)) & 1U;
            noiseSeed = (noiseSeed >> 1U) | (bits << 16U);
        }

        if (isMixerNoiseOnAnyChannel) {
            // Whenever the noise if "low shelf", simply cuts the signal.
            if (!isNoiseHighShelf) {
                if (isMixerNoiseAOpen) {
                    volumeA16Bits = 0U;
                }
                if (isMixerNoiseBOpen) {
                    volumeB16Bits = 0U;
                }
                if (isMixerNoiseCOpen) {
                    volumeC16Bits = 0U;
                }
            }
        }

        // Manages the sample. If present, they will overwrite the existing PSG values.
        playSample(channelASampleDataInfo, playSampleA, sampleAIndex, sampleAStep, volumeA16Bits, sampleRate, volumeTable16Bits);
        playSample(channelBSampleDataInfo, playSampleB, sampleBIndex, sampleBStep, volumeB16Bits, sampleRate, volumeTable16Bits);
        playSample(channelCSampleDataInfo, playSampleC, sampleCIndex, sampleCStep, volumeC16Bits, sampleRate, volumeTable16Bits);

        // Stores the data for the signal display.
        if (++signalCounter >= signalToStoreStepLimit) {
            signalChannelA.addData(volumeA16Bits);
            signalChannelB.addData(volumeB16Bits);
            signalChannelC.addData(volumeC16Bits);
            signalCounter = 0;
        }

        // Modifies the volumes of the muted channels. We do that after the signal display is done, so that the EQs can still show something
        // even if the channel is muted.
        if (isChannelAMuted) {
            volumeA16Bits = 0U;
        }
        if (isChannelBMuted) {
            volumeB16Bits = 0U;
        }
        if (isChannelCMuted) {
            volumeC16Bits = 0U;
        }

        // Mixes the channels.
        double leftVolume16Bits;    // NOLINT(*-init-variables)
        double rightVolume16Bits;   // NOLINT(*-init-variables)
        mixChannels(volumeA16Bits, volumeB16Bits, volumeC16Bits, leftVolume16Bits, rightVolume16Bits);

        // Converts to output volume from 0 to 1 max.
        // A + B mixed (or B mixed + C).
        constexpr auto maximum16BitsVolumePerOutput = static_cast<double>(Volumes::maximumValue) + (static_cast<double>(Volumes::maximumValue) / 2.0);
        const auto leftOutput = leftVolume16Bits / maximum16BitsVolumePerOutput;
        const auto rightOutput = rightVolume16Bits / maximum16BitsVolumePerOutput;
        jassert(leftOutput <= 1.0);
        jassert(rightOutput <= 1.0);

        *leftGenerationBufferPtrAdvancing = leftOutput;
        *rightGenerationBufferPtrAdvancing = rightOutput;

        ++leftGenerationBufferPtrAdvancing;
        ++rightGenerationBufferPtrAdvancing;

        ++generationBufferPosition;
    }
}

void PsgStreamGenerator::mixChannels(const uint16_t volumeA16Bits, const uint16_t volumeB16Bits, const uint16_t volumeC16Bits,
                                     double& leftVolume16BitsOut, double& rightVolume16BitsOut) const noexcept
{
    constexpr auto two = 2.0;
    constexpr auto maxChannel16BitValue = 65535.0;
    // Ratio to cram one channel and a half-one into one 16-bit value.
    constexpr auto oneAndHalfChannelRatio = maxChannel16BitValue / (maxChannel16BitValue + (maxChannel16BitValue / two));

    double leftChannel16Bits;   // NOLINT(*-init-variables)
    double middleChannel16Bits; // NOLINT(*-init-variables)
    double rightChannel16Bits;  // NOLINT(*-init-variables)

    switch (psgMixingOutput) {
        default:
            jassertfalse;       // A case not managed?
        case PsgMixingOutput::ABC: {
            leftChannel16Bits = volumeA16Bits;
            middleChannel16Bits = volumeB16Bits;
            rightChannel16Bits = volumeC16Bits;
            break;
        }
        case PsgMixingOutput::ACB: {
            leftChannel16Bits = volumeA16Bits;
            middleChannel16Bits = volumeC16Bits;
            rightChannel16Bits = volumeB16Bits;
            break;
        }
        case PsgMixingOutput::BAC: {
            leftChannel16Bits = volumeB16Bits;
            middleChannel16Bits = volumeA16Bits;
            rightChannel16Bits = volumeC16Bits;
            break;
        }
        case PsgMixingOutput::BCA: {
            leftChannel16Bits = volumeB16Bits;
            middleChannel16Bits = volumeC16Bits;
            rightChannel16Bits = volumeA16Bits;
            break;
        }
        case PsgMixingOutput::CAB: {
            leftChannel16Bits = volumeC16Bits;
            middleChannel16Bits = volumeA16Bits;
            rightChannel16Bits = volumeB16Bits;
            break;
        }
        case PsgMixingOutput::CBA: {
            leftChannel16Bits = volumeC16Bits;
            middleChannel16Bits = volumeB16Bits;
            rightChannel16Bits = volumeA16Bits;
            break;
        }
        case PsgMixingOutput::threeChannelsToLeft: {
            leftChannel16Bits = (volumeA16Bits + volumeB16Bits + volumeC16Bits) / 3.0;
            middleChannel16Bits = 0.0;
            rightChannel16Bits = 0.0;
            break;
        }
        case PsgMixingOutput::threeChannelsToMiddle: {
            leftChannel16Bits = 0.0;
            middleChannel16Bits = (volumeA16Bits + volumeB16Bits + volumeC16Bits) / 3.0;
            rightChannel16Bits = 0.0;
            break;
        }
        case PsgMixingOutput::threeChannelsToRight: {
            leftChannel16Bits = 0.0;
            middleChannel16Bits = 0.0;
            rightChannel16Bits = (volumeA16Bits + volumeB16Bits + volumeC16Bits) / 3.0;
            break;
        }
    }

    // Performs the balance between channels.
    leftChannel16Bits *= channelAMixVolume;
    middleChannel16Bits *= channelBMixVolume;
    rightChannel16Bits *= channelCMixVolume;

    const auto temp = middleChannel16Bits / two;
    leftVolume16BitsOut = (leftChannel16Bits + temp) * oneAndHalfChannelRatio;
    rightVolume16BitsOut = (rightChannel16Bits + temp) * oneAndHalfChannelRatio;

    jassert((leftVolume16BitsOut >= 0.0) && (leftVolume16BitsOut < 65536));
    jassert((rightVolume16BitsOut >= 0.0) && (rightVolume16BitsOut < 65536));
}

void PsgStreamGenerator::playSample(const SamplePlayInfo& sampleDataInfo, bool& playSample, double& sampleIndex, const double& sampleStep, uint16_t& volume16bitsOut,
                               double /*sampleRate*/, const std::array<uint16_t, Volumes::volumeCount>& volumeTable)
{
    // Don't play the sample? Then exits directly.
    if (!playSample) {
        return;
    }

    // Asked to play. There may be no sample though (to cut the previous sample). In this case, stops playing.
    if (!sampleDataInfo.isSamplePresent()) {
        playSample = false;
        return;
    }

    const auto& channelSample = sampleDataInfo.getSample();

    // "Dirtifies" the signal. --> Does not work well :(. Sounds strange. Strange, because the logs show that it is working.
    //auto sampleIndexDirty = sampleIndex;
    //const auto dirtyRatio = sampleRate / 22050.0;         // Hardcoded for now.
    //double errorShift = fmod(sampleIndexDirty, dirtyRatio);
    //const auto sampleIndexDirtyMod = sampleIndexDirty - errorShift;
    //DBG("AA" + juce::String(sampleIndex) + " --> " + juce::String(sampleIndexDirtyMod));
    //const auto sampleValue8bitsUnsignedChar = static_cast<unsigned char>(channelSample->getData()[static_cast<int>(sampleIndexDirtyMod)]);

    // Amplifies the sample, if needed.
    const auto amplificationRatio = sampleDataInfo.getAmplification();
    auto sampleValue8bitsUnsignedChar = static_cast<unsigned char>(channelSample->getData()[static_cast<int>(sampleIndex)]);
    if (!juce::exactlyEqual(amplificationRatio, 1.0F)) {
        sampleValue8bitsUnsignedChar = amplifySampleValue(sampleValue8bitsUnsignedChar, amplificationRatio);
    }

    // Decreases the sample from the note volume (from 0 to 15).
    auto sampleValue8bits = static_cast<int>(sampleValue8bitsUnsignedChar);
    const auto volume4bits = sampleDataInfo.getVolume4Bits();
    sampleValue8bits = sampleValue8bits * volume4bits / 15;

    // Applies the logarithmic table, but first scales to 0-31 to manage the half-step of the YM well.
    const auto volume5BitsScaled = scaleVolumeFrom0To16To0To32(static_cast<int>(static_cast<unsigned int>(sampleValue8bits) >> 4U));
    volume16bitsOut = volumeTable.at(static_cast<size_t>(volume5BitsScaled));

    // Manages the loop.
    const auto& loop = sampleDataInfo.getLoop();
    const auto isLooping = loop.isLooping();
    const auto pastMaxIndex = std::min(channelSample->getLength(), loop.getEndIndex() + 1);

    auto tempSampleIndex = sampleIndex + sampleStep;

    if (static_cast<int>(tempSampleIndex) >= pastMaxIndex) {
        tempSampleIndex = static_cast<double>(loop.getStartIndex());
        playSample = isLooping;
    }
    sampleIndex = tempSampleIndex;
}

int PsgStreamGenerator::scaleVolumeFrom0To16To0To32(const int volume0To16) noexcept
{
    jassert(volume0To16 <= 16);

    // 0 remains 0 to have an "electronic" 0.
    if (volume0To16 == 0) {
        return 0;
    }
    if (volume0To16 >= 16) {
        return 32;
    }

    return (volume0To16 * 2) + 1;     // Use the half-step to be able to reach the full volume.
}

void PsgStreamGenerator::readNextPsgRegisters() noexcept
{
    // Gets the Registers from the Provider.
    const auto registersAndSampleData = psgRegistersProvider.getNextRegisters(psgIndex);
    const auto registers = *registersAndSampleData.first;

    constexpr auto maximumSoftwarePeriodMask = static_cast<unsigned int>(PsgValues::maximumSoftwarePeriod);
    
    const auto newPeriodA = static_cast<int>(static_cast<unsigned int>(registers.getSoftwarePeriod(0)) & maximumSoftwarePeriodMask);
    const auto newPeriodB = static_cast<int>(static_cast<unsigned int>(registers.getSoftwarePeriod(1)) & maximumSoftwarePeriodMask);
    const auto newPeriodC = static_cast<int>(static_cast<unsigned int>(registers.getSoftwarePeriod(2)) & maximumSoftwarePeriodMask);
    //const auto isNewPeriodA = (periodA != newPeriodA);
    //const auto isNewPeriodB = (periodB != newPeriodB);
    //const auto isNewPeriodC = (periodC != newPeriodC);
    periodA = newPeriodA;
    periodB = newPeriodB;
    periodC = newPeriodC;

    constexpr auto maximumNoiseMask = static_cast<unsigned int>(PsgValues::maximumNoise);
    noise = static_cast<int>(static_cast<unsigned int>(registers.getNoise()) & maximumNoiseMask);
    noisePeriod = static_cast<int>(static_cast<unsigned int>(noise) << 1U);

    const auto mixer = static_cast<unsigned int>(registers.getMixer());
    isMixerSoundAOpen = ((mixer & 0x1U) == 0);
    isMixerSoundBOpen = ((mixer & 0x2U) == 0);
    isMixerSoundCOpen = ((mixer & 0x4U) == 0);
    isMixerNoiseAOpen = ((mixer & 0x8U) == 0);
    isMixerNoiseBOpen = ((mixer & 0x10U) == 0);
    isMixerNoiseCOpen = ((mixer & 0x20U) == 0);
    isMixerNoiseOnAnyChannel = isMixerNoiseAOpen || isMixerNoiseBOpen || isMixerNoiseCOpen;

    constexpr auto maximumVolumeMask = PsgValues::maskVolume;
    const auto volumeAFrom0To16 = static_cast<int>(static_cast<unsigned int>(registers.getVolume(0)) & maximumVolumeMask);
    const auto volumeBFrom0To16 = static_cast<int>(static_cast<unsigned int>(registers.getVolume(1)) & maximumVolumeMask);
    const auto volumeCFrom0To16 = static_cast<int>(static_cast<unsigned int>(registers.getVolume(2)) & maximumVolumeMask);
    // The volume stored is from 0 to 32 to manage YM half-step easily.
    volumeAFrom0To31Or32 = scaleVolumeFrom0To16To0To32(volumeAFrom0To16);
    volumeBFrom0To31Or32 = scaleVolumeFrom0To16To0To32(volumeBFrom0To16);
    volumeCFrom0To31Or32 = scaleVolumeFrom0To16To0To32(volumeCFrom0To16);

    constexpr auto maximumHardwarePeriodMask = static_cast<unsigned int>(PsgValues::maximumHardwarePeriod);
    const auto tempHardwarePeriod = static_cast<int>(static_cast<unsigned int>(registers.getHardwarePeriod()) & maximumHardwarePeriodMask); // NOT *2 because we want to manage half-steps for YM.
    // A bit hackish, but consider a period of 0 to be 1 (CRTC song bug), because it is the same on the real hardware (minus lowered volume if period is 0).
    hardwarePeriod = std::max(tempHardwarePeriod, minimumHardwarePeriod);

    // If the given R13 has a retrig, resets the Envelope counter.
    // Also, if the new R13 is different, the same.
    const auto hardwareEnvelopeAndRetrig = registers.getHardwareEnvelopeAndRetrig();
    const auto readHardwareEnvelope = static_cast<int>(static_cast<unsigned int>(hardwareEnvelopeAndRetrig.getEnvelope()) & PsgValues::maskHardwareEnvelope);
    jassert(readHardwareEnvelope >= 0);
    if (hardwareEnvelopeAndRetrig.isRetrig() || (readHardwareEnvelope != hardwareEnvelope)) {
        hardwareEnvelope = readHardwareEnvelope;
        hardwarePeriodCounter = 0;
        hardwareCurveCounter = 0;
    }

    // SIDs.
    prepareSid(registers.getSpecialEffects(0), isSidOnChannelA, sidLowShelfVolumeA, pendingPeriodAHighShelf, pendingPeriodALowShelf);
    prepareSid(registers.getSpecialEffects(1), isSidOnChannelB, sidLowShelfVolumeB, pendingPeriodBHighShelf, pendingPeriodBLowShelf);
    prepareSid(registers.getSpecialEffects(2), isSidOnChannelC, sidLowShelfVolumeC, pendingPeriodCHighShelf, pendingPeriodCLowShelf);

    // Now takes care of the possible sample.
    const auto* sampleData = registersAndSampleData.second.get();

    prepareSample(0, sampleData, sampleRate, samplePlayerFrequency, channelASampleDataInfo,
        sampleAIndex, playSampleA, sampleAStep, resamplingRatio);
    prepareSample(1, sampleData, sampleRate, samplePlayerFrequency, channelBSampleDataInfo,
        sampleBIndex, playSampleB, sampleBStep, resamplingRatio);
    prepareSample(2, sampleData, sampleRate, samplePlayerFrequency, channelCSampleDataInfo,
        sampleCIndex, playSampleC, sampleCStep, resamplingRatio);
}

void PsgStreamGenerator::prepareSample(const int channelIndex, const SampleData* newSampleData, const double sampleRate,
    const int samplePlayerFrequency, SamplePlayInfo& currentSampleDataInfo, double& currentSampleIndex,
    bool& currentPlaySample, double& currentSampleStep, const double resamplingRatio) noexcept
{
    // If the sample info requests that no sound must be emitted, do that and stop.
    if ((newSampleData == nullptr) || newSampleData->isMustStopAll()) {
        currentPlaySample = false;
        return;
    }

    // If no data is present for this channel, don't do anything...
    if (!newSampleData->isDataPresent(channelIndex)) {
        // ... unless there is a sample being played that is not high priority. In this case, we stop it.
        // This manages the case of a PSG note being played after a Sample note. The PSG note must be heard.
        if (currentPlaySample && !currentSampleDataInfo.isHighPriority()) {
            currentPlaySample = false;
        }
        return;
    }

    // Is there already a sample played? If it is high-priority, don't do anything, we don't want to replace it.
    // Unless the new sample is also high-priority!
    if (currentPlaySample && currentSampleDataInfo.isHighPriority() && !newSampleData->getSamplePlayInfo(channelIndex).isHighPriority()) {
        return;
    }

    const auto isSamplePresentNow = (currentSampleDataInfo.getSample() != nullptr);
    currentSampleDataInfo = newSampleData->getSamplePlayInfo(channelIndex);
    jassert(!currentSampleDataInfo.isEmpty());      // No empty data should have been added!

    if (!isSamplePresentNow || currentSampleDataInfo.isPlaySampleFromStart()) {
        currentSampleIndex = 0.0F;
    }
    if (currentSampleDataInfo.isSamplePresent() && currentSampleDataInfo.isPlaySampleFromStart()) {
        currentPlaySample = true;
    }

    if (currentPlaySample) {
        // Calculates the step at which the player must advance every frame.
        const auto referenceFrequency = currentSampleDataInfo.getReferenceFrequency();
        const auto newFrequency = currentSampleDataInfo.getPitchHz();
        const auto sampleFrequency = currentSampleDataInfo.getSampleFrequencyHz();

        // This allows to manage different tones from a same note from a 8Khz sample and a 44Khz one.
        const auto sampleFrequencyRatio =
            currentSampleDataInfo.mustForceSampleFrequencyForReplay() ? 1.0 :       // For SMA, we want this to be 1.0! (rather hackish...).
            static_cast<double>(samplePlayerFrequency) / sampleFrequency;
        const auto pitchFrequencyRatio = static_cast<double>(referenceFrequency) / newFrequency;

        auto sampleStep = (static_cast<double>(sampleFrequency) * sampleFrequencyRatio / sampleRate);
        sampleStep /= pitchFrequencyRatio;
        // Resampling ratio is useful to compensate the size of the buffer changing according to the PSG frequency.
        sampleStep /= resamplingRatio;

        currentSampleStep = sampleStep;
    }
}

void PsgStreamGenerator::fillSignal(const int channelIndex, const juce::HeapBlock<uint16_t>& blockToFill) const noexcept
{
    jassert((channelIndex >= 0) && (channelIndex <= 2));

    // What buffer to read?
    const auto& buffer = (channelIndex == 0) ? signalChannelA :
            (channelIndex == 1) ? signalChannelB : signalChannelC;

    // Go a bit before the current write position. An offset is used only because the given position is the NEXT position.
    const auto position = buffer.getOffsetInBuffer() - signalReadBufferSize - 1;
    buffer.fillFrom(position, blockToFill.getData(), signalReadBufferSize);
}

void PsgStreamGenerator::prepareSid(const SpecialEffect& specialEffect, bool& isSidOnChannel, int& sidLowShelfVolume, int& highShelfPeriod, int& lowShelfPeriod) noexcept
{
    isSidOnChannel = specialEffect.isSidActivated();
    sidLowShelfVolume = specialEffect.getLowShelfVolume();

    // On different note, restarts the SID. --> NO, else a vibrato wouldn't sound good.
    //if (isNewPeriod) {
        //isSidHighShelf = true;
        //currentSidPeriod = 0;
    //}

    highShelfPeriod = specialEffect.getHighShelfPeriod();
    lowShelfPeriod = specialEffect.getLowShelfPeriod();
}

void PsgStreamGenerator::manageSid(int& localVolumeFrom0To31Or32, const bool isSidOnChannel, bool& isSidHighShelf, const int sidLowShelfVolumeA, int& currentSidPeriod,
    int& periodHighShelf, int& periodLowShelf, const int pendingPeriodHighShelf, const int pendingPeriodLowShelf,
    const int psgFrequency, const SidPlayerCapability& sidPlayerCapability) noexcept
{
    if (!isSidOnChannel) {
        return;
    }

    constexpr auto sidMultiplier = 64;      // This sounds good, so that a ratio of 2 sounds like a normal SID.
    const auto shelfSidPeriod = (isSidHighShelf ? periodHighShelf : periodLowShelf) * sidMultiplier;
    auto sidPeriod = shelfSidPeriod;
    // TODO For ST, apply the whole process, get the ST MFP period, convert to Hz, convert it to ST Period?
    // The SID period must be aligned to the SID player accuracy.
    if (sidPlayerCapability.getSidPlayerProfile() != SidPlayerProfile::atariSt) {
        const auto sidAccuracyPeriod = static_cast<int>(static_cast<double>(psgFrequency) / sidPlayerCapability.getFrequencyHz());
        sidPeriod = static_cast<int>(std::round(static_cast<double>(shelfSidPeriod) / sidAccuracyPeriod) * sidAccuracyPeriod);
    }

    ++currentSidPeriod;
    if (currentSidPeriod >= sidPeriod) {
        currentSidPeriod = 0;
        isSidHighShelf = !isSidHighShelf;

        // We'll try to wait for a new cycle before changing the SID period.
        // If we are now at a high-front, it means a new cycle started, we can switch to the next period.
        if (isSidHighShelf) {
            periodHighShelf = pendingPeriodHighShelf;
            periodLowShelf = pendingPeriodLowShelf;
        }
    }

    // If low shelf, uses the "off" volume.
    if (!isSidHighShelf) {
        localVolumeFrom0To31Or32 = sidLowShelfVolumeA * 2;
    }
}

uint16_t PsgStreamGenerator::manageHardwareSound(const bool isHardwareEnvelopeUsed, const bool isSidOnChannel, const bool isSidHighShelf,
    const bool isMixerSoundOpen, const bool isHighShelf, const HardwareTables& hardwareTable,
    const int volume5Bits, const std::array<uint16_t, Volumes::volumeCount>& volumeTable16Bits, const int hardwareEnvelope, const int hardwareCurveCounter) noexcept
{
    uint16_t volumeA16Bits;   // NOLINT(*-init-variables)
    if (isHardwareEnvelopeUsed) {
        // Hardware sound. If the mixer is on, the volume depends on the high/low shelf of the software envelope, but the high shelf volume is defined by the hardware envelope.
        // If the mixer is off. The volume is only given by the hardware envelope. No more high/low shelf.
        auto newVolume5Bits = ((isMixerSoundOpen && isHighShelf) || !isMixerSoundOpen) ? hardwareTable.getVolume(hardwareEnvelope, hardwareCurveCounter) : 0;
        // If SID low shelf, uses the volume 0 which has been set by SID.
        if (isSidOnChannel && !isSidHighShelf) {
            newVolume5Bits = volume5Bits;
        }

        volumeA16Bits = volumeTable16Bits.at(static_cast<size_t>(newVolume5Bits));
    } else {
        // Software sound. If mixer on, the volume depends on the shelf state, else, it is directly the volume itself.
        volumeA16Bits = ((isMixerSoundOpen && isHighShelf) || !isMixerSoundOpen) ? volumeTable16Bits.at(static_cast<size_t>(volume5Bits)) : static_cast<uint16_t>(0);
    }

    return volumeA16Bits;
}

}   // namespace arkostracker
