#include "YmReader.h"

#include "../../utils/PsgValues.h"
#include "decoder/YmDecoder.h"
#include "decoder/YmDecoder3b.h"
#include "decoder/YmDecoder456.h"

namespace arkostracker 
{

YmReader::YmReader(juce::InputStream& pInputStream) noexcept:
        StreamedMusicReader(pInputStream),
        ymDecoder(),
        ymMetadata(),
        iterationToHardwareEnvelope()
{
}


// StreamedMusicReader method implementations.
// ===================================================

bool YmReader::checkFormat() noexcept
{
    // What decoder fits?
    auto& stream = getInputStream();
    std::vector<std::unique_ptr<YmDecoder>> decoders;
    decoders.emplace_back(std::make_unique<YmDecoder123>(stream));
    decoders.emplace_back(std::make_unique<YmDecoder3b>(stream));
    decoders.emplace_back(std::make_unique<YmDecoder456>(stream));

    auto foundDecoder = false;
    for (auto& decoder : decoders) {
        if (decoder->acceptFormatHeader()) {
            // This Decoder accepts the format! We can stop and use this decoder.
            ymDecoder = std::move(decoder);
            foundDecoder = true;
            break;
        }
    }
    if (!foundDecoder) {
        return false;
    }

    // Parses the header to get the metadata. An error can still occur.
    auto success = false;
    ymMetadata = ymDecoder->parseHeader(success);
    return success;
}

bool YmReader::prepare() noexcept
{
    buildHardwareEnvelopesForIterations();
    return true;
}

PsgType YmReader::getPsgType(const int psgIndex) const noexcept
{
    jassert(psgIndex == 0); (void)psgIndex;     // Only one PSG in YM!

    // There are no metadata about the PSG type, so checks the PSG frequency and deduces it.
    return (ymMetadata.psgMasterClockHz == static_cast<int>(PsgFrequency::psgFrequencyAtariST)) ? PsgType::ym : PsgType::ay;
}

int YmReader::getPsgCount() const noexcept
{
    return 1;
}

int YmReader::getIterationCount() const noexcept
{
    return ymMetadata.iterationCount;
}

int YmReader::getDigidrumCount() const noexcept
{
    return ymMetadata.getDigidrumCount();
}

int YmReader::getLoopIndex() const noexcept
{
    return ymMetadata.loopIterationIndex;
}

int YmReader::getPsgFrequencyHz(const int psgIndex) const
{
    jassert(psgIndex == 0); (void)psgIndex;     // Only one PSG in YM!

    return ymMetadata.psgMasterClockHz;
}

float YmReader::getPlayerReplayHz() const
{
    return static_cast<float>(ymMetadata.playerReplayHz);
}

juce::String YmReader::getTitle() const
{
    return ymMetadata.title;
}

juce::String YmReader::getAuthor() const
{
    return ymMetadata.author;
}

juce::String YmReader::getComments() const
{
    return ymMetadata.comments;
}

SamplePlayInfo YmReader::getSamplePlayInfo(const int sampleIndex, const int sampleReplayFrequencyHz) const
{
    const auto& samples = ymMetadata.digidrums;
    if (sampleIndex >= static_cast<int>(samples.size())) {
        return { };
    }

    const auto sample = samples.at(static_cast<size_t>(sampleIndex));

    const auto loop = Loop(0, sample->getLength() - 1, false);
    constexpr auto volume4bits = PsgValues::maximumVolumeNoHard;
    constexpr auto pitchHz = 440.0F;
    constexpr auto highPriority = true;
    // Important, else the sample replay is not right (test with Turrican 2 songs and Chamber of Shaolin / Mega Pock Olipse).
    constexpr auto forceSampleFrequencyForReplay = true;
    constexpr auto amplification = 1.0F;

    return { sample, true, amplification, loop, sampleReplayFrequencyHz, pitchHz,
        PsgFrequency::defaultReferenceFrequencyHz, volume4bits, highPriority,
        forceSampleFrequencyForReplay
    };
}

std::pair<PsgRegisters, bool> YmReader::getRegisters(const int psgIndex, const int iterationIndex) const noexcept
{
    jassert(ymDecoder != nullptr);

    PsgRegisters psgRegisters;

    if (psgIndex != 0) {
        jassertfalse;     // Only one PSG in YM!
        return { psgRegisters, false };
    }

    auto success = true;

    // Checks 16 registers.
    for (auto reg = 0, registerCount = ymDecoder->getEncodedRegisterCountInFrame(); reg < registerCount; ++reg) {
        // The R13 has a special method to get the latest value, and the retrig.
        unsigned char value;            // NOLINT(*-init-variables)
        if (reg != 13) {
            value = ymDecoder->getValue(iterationIndex, reg, success);
        } else {
            // R13. Gets the R13, but also the Retrig.
            auto valueAndRetrig = getHardwareEnvelopeAndRetrig(iterationIndex);
            value = static_cast<unsigned char>(valueAndRetrig.getEnvelope());
            psgRegisters.setRetrig(valueAndRetrig.isRetrig());
        }

        jassert(success);
        if (success) {
            psgRegisters.setValue(reg, value);
        }
    }

    return { psgRegisters, success };
}

bool YmReader::areDigidrumsAllowed() const
{
    return true;
}

std::vector<std::shared_ptr<Sample>> YmReader::getDigidrums() const noexcept
{
    return ymMetadata.digidrums;
}

// ===================================================

void YmReader::buildHardwareEnvelopesForIterations() noexcept
{
    jassert(ymDecoder != nullptr);

    auto success = false;
    iterationToHardwareEnvelope.clear();

    // Reads all the R13, continuously.
    unsigned char previousR13Value = 16U;        // Sentinel value.
    for (auto iterationIndex = 0, iterationCount = ymMetadata.iterationCount; iterationIndex < iterationCount; ++iterationIndex) {
        const auto reg13value = ymDecoder->getValue(iterationIndex, 13, success);
        jassert(success);
        if (success) {
            // If R13 is the same, it means it must be retrigged.
            // If R13 is not the same, it also must be stored, but without retrig.
            // If R13 is 255, it means the value doesn't change, no retrig (so don't do anything).
            if (reg13value != 255) {
                jassert(iterationToHardwareEnvelope.find(iterationIndex + 1) == iterationToHardwareEnvelope.cend());    // This one should never be set already!
                const auto retrig = (previousR13Value == reg13value);
                iterationToHardwareEnvelope.insert(std::make_pair(iterationIndex, HardwareEnvelopeAndRetrig(reg13value, retrig)));

                previousR13Value = reg13value;
            }
        }
    }

    // Security, in the (rather unlikely) case there is no hardware envelope.
    if (iterationToHardwareEnvelope.empty()) {
        iterationToHardwareEnvelope.insert(std::make_pair(0, HardwareEnvelopeAndRetrig::buildDefault()));
    }
}

HardwareEnvelopeAndRetrig YmReader::getHardwareEnvelopeAndRetrig(const int iterationIndex) const noexcept
{
    // Shouldn't be empty! If it is, returns a fallback value.
    jassert(!iterationToHardwareEnvelope.empty());      // Did you call the buildHardwareEnvelopesForIterations method first?
    if (iterationToHardwareEnvelope.empty()) {
        return HardwareEnvelopeAndRetrig::buildDefault();
    }

    // Finds the closest iteration in the map. Searches for the one equal OR LOWER.
    auto foundEqual = false;
    auto iterator = iterationToHardwareEnvelope.lower_bound(iterationIndex);
    if (iterator == iterationToHardwareEnvelope.cend()) {
        // Not found? More likely, we have reached the end. Use the last one. Safe because we know the map is not empty.
        iterator = --iterationToHardwareEnvelope.cend();
    } else {
        // Found. Is it equal? If yes, we can use the iterator.
        if (iterator->first != iterationIndex) {
            // Else, we use the previous item, if we can!
            if (iterator != iterationToHardwareEnvelope.cbegin()) {
                --iterator;
            }
        } else {
            foundEqual = true;
        }
    }

    auto hardwareEnvelopeAndRetrig = iterator->second;

    // If a "non-equal" was found, removes the retrig, if any.
    // This is needed for fast forwarding, else the same env+retrig was can be used: the retrig will be called at every next frames.
    if (!foundEqual && hardwareEnvelopeAndRetrig.isRetrig()) {
        hardwareEnvelopeAndRetrig.setRetrig(false);
    }

    return hardwareEnvelopeAndRetrig;
}

}   // namespace arkostracker
