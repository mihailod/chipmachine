#include "YmStreamEncoder.h"

#include "../../business/instrument/SampleResampler.h"
#include "../samples/SampleEncoder.h"

namespace arkostracker 
{

YmStreamEncoder::YmStreamEncoder(std::shared_ptr<const Song> pSong, Id pSubsongId, const int pPsgIndex, const bool pIsYm3,
    const OptionalInt pTargetPsgFrequency) noexcept :
        song(std::move(pSong)),
        subsongId(std::move(pSubsongId)),
        psgIndex(pPsgIndex),
        isYm3(pIsYm3),
        targetPsgFrequencyHz(pTargetPsgFrequency),
        eventToYmDigidrum(),
        sampleIndexes()
{
}


// ========================================================

void YmStreamEncoder::encodeRegisters(juce::OutputStream& outputStream, const PsgRegisters& psgRegisters, const int previousHardwareEnvelope) noexcept
{
    // Encodes each registers.
    outputStream.writeShort(static_cast<int16_t>(psgRegisters.getSoftwarePeriod(0)));            // Regs 0, 1.
    outputStream.writeShort(static_cast<int16_t>(psgRegisters.getSoftwarePeriod(1)));            // Regs 2, 3.
    outputStream.writeShort(static_cast<int16_t>(psgRegisters.getSoftwarePeriod(2)));            // Regs 4, 5.
    outputStream.writeByte(static_cast<char>(psgRegisters.getNoise()));                                     // Regs 6.
    outputStream.writeByte(static_cast<char>(psgRegisters.getMixer()));                                     // Regs 7.
    outputStream.writeByte(static_cast<char>(psgRegisters.getVolume(0)));                         // Regs 8.
    outputStream.writeByte(static_cast<char>(psgRegisters.getVolume(1)));                         // Regs 9.
    outputStream.writeByte(static_cast<char>(psgRegisters.getVolume(2)));                         // Regs 10.
    outputStream.writeShort(static_cast<int16_t>(psgRegisters.getHardwarePeriod()));                        // Regs 11, 12.
    // If no retrig, encodes 255. It is supposed that if the value changes, the retrig flag is set to true.
    const auto hardwareEnvelopeAndRetrig = psgRegisters.getHardwareEnvelopeAndRetrig();
    const auto envelope = static_cast<signed char>(hardwareEnvelopeAndRetrig.getEnvelope());
    const auto envelopeChar = static_cast<char>(envelope);
    outputStream.writeByte(hardwareEnvelopeAndRetrig.isRetrig() ? envelopeChar
        // No retrig. Did the envelope changed? If no, encodes 255.
        : (envelopeChar == static_cast<char>(previousHardwareEnvelope)) ? static_cast<char>(255) : envelopeChar);

    outputStream.writeByte(static_cast<char>(psgRegisters.getValueFromRegister(14)));     // Not useful.
    outputStream.writeByte(static_cast<char>(psgRegisters.getValueFromRegister(15)));     // Useful for digidrums.
}

void YmStreamEncoder::encodeByte(juce::OutputStream& outputStream, const int value) noexcept
{
    outputStream.writeByte(static_cast<char>(value));
}

int YmStreamEncoder::getDigidrumCount() const noexcept
{
    return static_cast<int>(sampleIndexes.size());
}

void YmStreamEncoder::fillPsgRegistersWithDigidrumData(PsgRegisters& psgRegisters, const int digiChannel) noexcept
{
    // Any digidrum from event? It is mapped to an increasing number.
    const auto event = psgRegisters.getEventNumber();
    if (event == 0) {
        return;
    }

    // Is this event is a sample? It could be a simple event, and not a sample!
    auto foundSampleInstrument = false;
    song->performOnConstInstrumentFromIndex(event, [&](const Instrument& instrument) {
        foundSampleInstrument = (instrument.getType() == InstrumentType::sampleInstrument);
    });

    if (!foundSampleInstrument) {
        return;
    }

    // Does this event already exist in the digidrums map? If not, maps it.
    size_t ymDigidrumIndex;            // NOLINT(*-init-variables)
    if (const auto iterator = eventToYmDigidrum.find(event); iterator == eventToYmDigidrum.cend()) {
        ymDigidrumIndex = eventToYmDigidrum.size();
        eventToYmDigidrum.insert(std::make_pair(event, static_cast<int>(ymDigidrumIndex)));
        sampleIndexes.push_back(event);     // Handy to encode the digidrums without having to sort anything.
    } else {
        ymDigidrumIndex = static_cast<size_t>(iterator->second);
    }

    // Only 32 digidrums are authorized (0-31).
    if (ymDigidrumIndex > 31) {
        jassertfalse;       // Too many digidrums! Ignoring the new ones.
        return;
    }

    // Gets the sample frequency of the sample.
    auto sampleFrequencyHz = PsgFrequency::defaultSampleFrequencyHz;
    song->performOnConstSampleInstrumentFromIndex(event, [&](const Instrument&, const SamplePart& samplePart) noexcept {
        sampleFrequencyHz = samplePart.getFrequencyHz();
    });

    // Calculates the R15/Volume registers.
    // See the YM doc about the TP (timer predivisor) and TC (timer count).
    // MFP runs at 2457600 hz. TP is fixed to 0b001, hence prediv by 4 (see the doc for the translation), as /4 is enough for us. This simplifies things.

    // SampleFrequency = 2457600 / TP / TC.
    // TC = 2457600 / TP / SampleFrequency.
    // TC = 2457600 / 4 / SampleFrequency.
    // TC = 614400 / SampleFrequency.
    auto r8 = static_cast<unsigned char>(static_cast<unsigned int>(psgRegisters.getVolume(0)) | (0b001U << 5U));
    const auto r15 = static_cast<unsigned char>(614400.0 / static_cast<double>(sampleFrequencyHz));

    // R3's bit 5/4 indicates on which channel the digidrum is played.
    // Bit 7/6:
    // 00: SID
    // 01: Digidrum
    // 10 : Sinus - SID
    // 11 : Sync - Buzzer
    const auto r3DigidrumMask = (0b0100U | static_cast<unsigned int>((digiChannel + 1))) << 4U;

    const auto r3 = static_cast<unsigned char>(static_cast<unsigned int>(psgRegisters.getValueFromRegister(3)) & 0b11111U);
    auto r9 = static_cast<unsigned char>(psgRegisters.getVolume(1));
    auto r10 = static_cast<unsigned char>(psgRegisters.getVolume(2));

    // Applies the digidrum index on the volume (the volume is erased).
    if (digiChannel == 0) {
        r8 = static_cast<unsigned char>((r8 & 0b11100000U) | ymDigidrumIndex);
    } else if (digiChannel == 1) {
        r9 = static_cast<unsigned char>((r9 & 0b11100000U) | ymDigidrumIndex);
    } else if (digiChannel == 2) {
        r10 = static_cast<unsigned char>((r10 & 0b11100000U) | ymDigidrumIndex);
    }

    psgRegisters.setValue(3, static_cast<int>(r3 | r3DigidrumMask));
    psgRegisters.setValue(8, r8);
    psgRegisters.setValue(9, r9);
    psgRegisters.setValue(10, r10);
    psgRegisters.setValue(14, 0);        // Nothing to encode here.
    psgRegisters.setValue(15, r15);
}

void YmStreamEncoder::encodeDigidrums(juce::OutputStream& outputStream) const noexcept
{
    jassert(sampleIndexes.size() == eventToYmDigidrum.size());

    constexpr auto fadeOutLength = 20;      // Cleaner.
    const SampleEncoderFlags sampleEncodeFlags(true, 256, 0, 0,
                                               fadeOutLength, true, true);
    const SampleEncoder sampleEncoder(sampleEncodeFlags);

    for (const auto instrumentIndex : sampleIndexes) {
        song->performOnConstSampleInstrumentFromIndex(instrumentIndex, [&](const Instrument&, const SamplePart& nonResampledSamplePart) {
            // Important to apply the resampling first.
            const auto samplePart = SampleResampler::resample(nonResampledSamplePart);

            const auto originalSample = samplePart.getSample();
            const auto& originalSampleData = originalSample->getData();

            const auto amplification = samplePart.getAmplificationRatio();
            const auto sampleToEncodeLastIndex = samplePart.getLoop().getEndIndex();
            const auto sampleToEncodeSize = sampleToEncodeLastIndex + 1;

            // Applies the amplification, reduces to the used data.
            const auto encodedSampleData = sampleEncoder.convert(originalSampleData, amplification, false, 0, sampleToEncodeLastIndex);
            jassert(sampleToEncodeLastIndex == static_cast<int>(encodedSampleData->size() - 1));

            // Encodes the header.
            outputStream.writeIntBigEndian(sampleToEncodeSize);
            // Encodes the sample data.
            for (auto index = 0, size = sampleToEncodeSize; index < size; ++index) {
                const auto sample = encodedSampleData->at(static_cast<size_t>(index));
                outputStream.writeByte(static_cast<char>(sample));
            }
        });
    }
}

}   // namespace arkostracker
