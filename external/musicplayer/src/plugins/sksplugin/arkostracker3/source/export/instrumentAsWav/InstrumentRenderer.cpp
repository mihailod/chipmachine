#include "InstrumentRenderer.h"

#include "../../audio/sources/PsgStreamGenerator.h"
#include "../../business/instrument/SampleResampler.h"
#include "../../player/CellToPlay.h"

namespace arkostracker
{

InstrumentRenderer::InstrumentRenderer(const std::shared_ptr<Song>& pSong, const Psg& pPsg, const SidPlayerCapability& pSidPlayerCapability) noexcept :
        song(pSong),
        psg(pPsg),
        sidPlayerCapability(pSidPlayerCapability),
        emptyInstrumentId(song->getInstrumentId(0).getValue()),
        iterationCount(),
        channelPlayer(*this, 0),
        canceled()
{
}

void InstrumentRenderer::setCanceled() noexcept
{
    canceled = true;
}

int InstrumentRenderer::renderNote(const int note, const Id& instrumentId, juce::OutputStream& outputStream,
    const float replayFrequencyHz, const int psgSampleRate, const int minimumDurationMs, const OptionalInt maximumSize) noexcept
{
    auto instrumentType = InstrumentType::psgInstrument;
    song->performOnConstInstrument(instrumentId, [&] (const Instrument& instrument) {
        instrumentType = instrument.getType();
    });

    const auto outputSampleRate = getOutputSampleRate(*song, instrumentId, instrumentType, psgSampleRate);

    switch (instrumentType) {
        case InstrumentType::psgInstrument:
            renderNoteForPsgInstrument(note, instrumentId, outputStream, replayFrequencyHz, psgSampleRate, minimumDurationMs);
            break;
        case InstrumentType::sampleInstrument:
            renderNoteForSampleInstrument(*song, note, instrumentId, outputStream, maximumSize);
            break;
        default:
            jassertfalse;       // Instrument type not managed!
            break;
    }

    outputStream.flush();

    return outputSampleRate;
}

int InstrumentRenderer::getOutputSampleRate(const Song& song, const Id& instrumentId, const InstrumentType instrumentType, const int psgSampleRate) noexcept
{
    if (instrumentType != InstrumentType::sampleInstrument) {
        return psgSampleRate;
    }

    // The output rate for sample is the same as the sample itself.
    auto sampleRate = psgSampleRate;
    song.performOnConstInstrument(instrumentId, [&] (const Instrument& instrument) {
        jassert(instrument.getType() == InstrumentType::sampleInstrument);

        sampleRate = instrument.getConstSamplePart().getFrequencyHz();
    });

    return sampleRate;
}

void InstrumentRenderer::renderNoteForSampleInstrument(const Song& song, const int note, const Id& instrumentId,
    juce::OutputStream& outputStream, const OptionalInt maximumSize) noexcept
{
    juce::MemoryBlock originalSample;
    song.performOnConstInstrument(instrumentId, [&] (const Instrument& instrument) {
        jassert(instrument.getType() == InstrumentType::sampleInstrument);

        originalSample = instrument.getConstSamplePart().getSample()->getData();
    });

    // Resamples the sample.
    const auto& [outputSample, _] = SampleResampler::resample(originalSample, static_cast<int>(originalSample.getSize()), note, 0.0);

    // The sample is 8 bits, unsigned. Makes it signed, and to float.
    // Also trims the size, if wanted.
    auto originalSize = static_cast<int>(outputSample.getSize());
    if (maximumSize.isPresent()) {
        originalSize = std::min(originalSize, maximumSize.getValue());
    }
    const auto size = ((originalSize & 1) != 0) ? originalSize - 1 : originalSize ;     // Incredibly enough, if size is odd, some WAV readers cannot read it.
    for (auto index = 0; index < size; ++index) {
        const auto newValue = static_cast<unsigned char>(outputSample[index]);
        const auto newValueFloat = (newValue - 128) / 128.0F;
        outputStream.writeFloat(newValueFloat);
    }
}

void InstrumentRenderer::renderNoteForPsgInstrument(const int note, const Id& instrumentId, juce::OutputStream& outputStream,
    float replayFrequencyHz, const int psgSampleRate, const int minimumDurationMs) noexcept
{
    constexpr auto psgIndex = 0;
    constexpr auto channelsMixVolume = 1.0F;
    constexpr auto bufferSize = 512;

    iterationCount = 0;

    // How long is the sound?
    auto pastLastIterationIndex = 0;
    const auto instrumentIndex = song->getInstrumentIndex(instrumentId);
    if (instrumentIndex.isAbsent()) {
        return;
    }
    song->performOnConstInstrument(instrumentId, [&] (const Instrument& instrument) {
        const auto& psgPart = instrument.getConstPsgPart();
        const auto instrumentEndIndex = psgPart.getMainLoopRef().getEndIndex();
        pastLastIterationIndex = (instrumentEndIndex + 1) * (psgPart.getSpeed() + 1);
    });

    PsgStreamGenerator psgStreamGenerator(*this, psg.getType(), psgIndex, replayFrequencyHz,
                                          psg.getPsgFrequency(), psg.getSamplePlayerFrequency(), PsgMixingOutput::ABC,
                                          channelsMixVolume, channelsMixVolume, channelsMixVolume, sidPlayerCapability);

    psgStreamGenerator.prepareToPlay(bufferSize, psgSampleRate);

    constexpr auto outputChannelCount = 2;      // The PSG generator only works with stereo.
    juce::AudioBuffer<float> bufferToUse(outputChannelCount, bufferSize);
    const juce::AudioSourceChannelInfo bufferToFill(bufferToUse);

    const auto cell = Cell::build(note, instrumentIndex.getValue());
    const CellToPlay cellToPlay(cell, channelIndex, { });
    channelPlayer.postCell(cellToPlay);

    // Forces to a minimum duration, if present.
    if (minimumDurationMs > 0) {
        const auto msPerFrame = 1.0F / replayFrequencyHz * 1000;
        const auto minimumIterationCount = static_cast<int>(static_cast<double>(minimumDurationMs) / msPerFrame);
        pastLastIterationIndex = std::max(pastLastIterationIndex, minimumIterationCount);
    }

    while ((iterationCount < pastLastIterationIndex) && !canceled) {
        bufferToFill.clearActiveBufferRegion();     // Security, but doesn't seem useful.

        psgStreamGenerator.getNextAudioBlock(bufferToFill);

        // Probably not optimized, could use some raw pointers...
        for (auto index = 0; index < bufferSize; ++index) {
            const auto value = bufferToUse.getSample(channelIndex, index);
            outputStream.writeFloat(value);
        }
    }

    psgStreamGenerator.releaseResources();
}


// PsgRegistersProvider method implementations.
// ================================================

std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> InstrumentRenderer::getNextRegisters(const int psgIndex) noexcept
{
    jassert(psgIndex == 0); (void)psgIndex;

    const auto result = channelPlayer.playStream(iterationCount == 0, false);

    const auto psgRegisters = result->getChannelOutputRegisters().toPsgRegisters(channelIndex);

    ++iterationCount;

    return { std::make_unique<PsgRegisters>(psgRegisters), nullptr };
}


// SongDataProvider method implementations.
// ================================================

SongDataProvider::PsgInstrumentFrameData InstrumentRenderer::getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, const int cellIndex) const noexcept
{
    if (instrumentId.isAbsent() || (instrumentId == emptyInstrumentId)) {
        return { Loop(0, 0, true), 0, false, LowLevelPsgInstrumentCell() };
    }

    std::unique_ptr<PsgInstrumentFrameData> psgInstrumentFrameData;
    song->performOnConstInstrument(instrumentId.getValue(), [&] (const Instrument& instrument) {
        jassert(instrument.getType() == InstrumentType::psgInstrument);
        const auto& psgPart = instrument.getConstPsgPart();

        const auto lowLevelCell = psgPart.buildLowLevelCell(cellIndex);
        const auto speed = psgPart.getSpeed();
        const auto loop = psgPart.getMainLoop();
        const auto isInstrumentRetrig = psgPart.isInstrumentRetrig();

        psgInstrumentFrameData = std::make_unique<PsgInstrumentFrameData>(loop, speed, isInstrumentRetrig, lowLevelCell);
    });

    return *psgInstrumentFrameData;
}

OptionalId InstrumentRenderer::getInstrumentIdFromAudioThread(const int instrumentIndex) const noexcept
{
    return song->getInstrumentId(instrumentIndex);
}

InstrumentType InstrumentRenderer::getInstrumentTypeFromAudioThread(const OptionalId& /*instrumentId*/) const noexcept
{
    return InstrumentType::psgInstrument;
}

OptionalId InstrumentRenderer::getExpressionIdFromAudioThread(bool /*isArpeggio*/, int /*expressionIndex*/) const noexcept
{
    return { };
}

SongDataProvider::ExpressionMetadata InstrumentRenderer::getExpressionMetadataFromAudioThread(bool /*isArpeggio*/, const OptionalId& /*expressionId*/) const noexcept
{
    return { 0, 0, 0 };
}

SongDataProvider::SampleInstrumentFrameData InstrumentRenderer::getSampleInstrumentFrameDataFromAudioThread(const OptionalId& /*instrumentId*/) const noexcept
{
    return { Loop(), 0, 1.0F, nullptr };
}

int InstrumentRenderer::getExpressionValueFromAudioThread(bool /*isArpeggio*/, const OptionalId& /*expressionId*/, int /*cellIndex*/) const noexcept
{
    return 0;
}

std::pair<int, float> InstrumentRenderer::getPsgFrequencyFromChannelFromAudioThread(int /*channelIndexInSong*/) const noexcept
{
    return { psg.getPsgFrequency(), psg.getReferenceFrequency() };
}

int InstrumentRenderer::getTranspositionFromAudioThread(int /*channelIndexInSong*/) const noexcept
{
    return 0;
}

bool InstrumentRenderer::isEffectContextEnabled() const noexcept
{
    return false;
}

LineContext InstrumentRenderer::determineEffectContextFromAudioThread(CellLocationInPosition /*location*/) const noexcept
{
    return {};
}

LineContext InstrumentRenderer::determineEffectContextFromAudioThread(int /*channelIndexInSong*/) const noexcept
{
    return {};
}

SidPlayerCapability InstrumentRenderer::getSidPlayerCapability() const noexcept
{
    return sidPlayerCapability;
}

}   // namespace arkostracker
