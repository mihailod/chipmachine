#include "Vt2InstrumentBuilder.h"

#include <utility>

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"

namespace arkostracker 
{

Vt2InstrumentBuilder::Vt2InstrumentBuilder(SongBuilder& pSongBuilder, juce::String pInstrumentName) noexcept :
        songBuilder(pSongBuilder),
        instrumentName(std::move(pInstrumentName)),
        lines(),
        currentShiftPeriod(0),
        currentShiftNoise(0),
        currentShiftVolume(0)
{
}

void Vt2InstrumentBuilder::addInstrumentLine(const juce::String& line) noexcept
{
    lines.push_back(line);
}

bool Vt2InstrumentBuilder::parseEnded() noexcept
{
    jassert(!lines.empty());

    songBuilder.setCurrentInstrumentName(instrumentName);

    auto instrumentHeight = 0;

    // Finds the loop, if any.
    auto loopToIndexAndLoop = findLoopToIndex();
    auto loopToIndex = loopToIndexAndLoop.first;
    auto isLooping = loopToIndexAndLoop.second;

    auto success = true;

    // First, parses the lines from 0 to the LoopStart *excluded*. There might be no line to parse.
    // This is to simply to code and the management of the loop.
    for (auto lineIndex = 0; lineIndex < loopToIndex; ++lineIndex) {
        const auto& line = lines.at(static_cast<size_t>(lineIndex));
        bool foundIncDec;  // We don't care about it for this pass.    // NOLINT(*-init-variables)
        auto cell = parseInstrumentLine(line, success, foundIncDec);
        if (!success) {
            return false;
        }
        encodeInstrumentCell(cell);
        ++instrumentHeight;
    }

    // Parses all the lines, in maybe several passes, from the loopStart to the end index,
    // for as long as the generated lines are different as the other, because it means the sound still evolves.
    // We put a limit to this, because the result may be endless, which is not problematic in VT2, but is in AT2.
    std::vector<PsgInstrumentCell> previousGeneratedLoopCells;

    auto mustContinue = true;
    while (mustContinue) {
        std::vector<PsgInstrumentCell> generatedLoopCells;

        auto foundIncDec = false;           // Found any increase/decrease in pitch/noise/volume?
        // Makes one pass and stores the cells.
        for (auto lineIndex = loopToIndex, height = static_cast<int>(lines.size()); lineIndex < height; ++lineIndex) {
            const auto& line = lines.at(static_cast<size_t>(lineIndex));
            auto cell = parseInstrumentLine(line, success, foundIncDec);
            if (!success) {
                return false;
            }
            generatedLoopCells.push_back(cell);

            // As soon as an inc/dec is found, the loop is removed, we are going to generate as many lines as needed.
            if (foundIncDec) {
                isLooping = false;
            }
        }

        // Should we continue? Only if the new Cells are different from the previous ones.
        // On first pass, this will always be true, which will make the encoding of the Cells from loop start to the end. Handy.
        mustContinue = areCellsDifferent(previousGeneratedLoopCells, generatedLoopCells);
        if (mustContinue) {
            for (const auto& cell : generatedLoopCells) {
                encodeInstrumentCell(cell);
                ++instrumentHeight;

                // Checks height limit!
                mustContinue = (instrumentHeight < 64);
                if (!mustContinue) {
                    break;
                }
            }
        }

        // If the sound is looping, we have already encoded what is needed, we can stop.
        if (isLooping) {
            mustContinue = false;
        }

        if (mustContinue) {
            previousGeneratedLoopCells = generatedLoopCells;
        }
    }

    // The encoding is finished.
    // Sets the loop/height.
    const auto cellLastIndex = instrumentHeight - 1;
    songBuilder.setCurrentPsgInstrumentMainLoop(loopToIndex, cellLastIndex, isLooping);

    songBuilder.finishCurrentInstrument();

    return true;
}

PsgInstrumentCell Vt2InstrumentBuilder::parseInstrumentLine(const juce::String& line, bool& successOutput, bool& foundIncDecOutput) noexcept
{
    successOutput = true;
    foundIncDecOutput = false;

    // Example:
    // T.. +000_ +00_ A_ L
    // T.. +002^ +03^ F+
    // TNE -066^ +00^ F-
    // T.. -002^ -01_ 1+
    // 01234567890123456789

    const auto lineLength = line.length();
    jassert(lineLength >= 17);           // Without "L".
    if (lineLength < 17) {
        successOutput = false;
        return {};
    }

    auto index = 0;
    const auto isTone = (line[index++] == 'T');
    const auto isNoise = (line[index++] == 'N');
    index += 2;     // Skips E and the space.

    // Parses the shift period.
    const auto isShiftPeriodNegative = (line[index] == '-');
    const auto shiftPeriodHexString = line.substring(index + 1, index + 4);
    auto shiftPeriod = shiftPeriodHexString.getHexValue32();
    if (isShiftPeriodNegative) {
        shiftPeriod = -shiftPeriod;
    }
    const auto isShiftPeriodAddition = (line[index + 4] == '^');
    index += 6;

    // Parses the noise.
    const auto noiseHexString = line.substring(index + 1, index + 3);
    auto noise = noiseHexString.getHexValue32();
    if (line[index] == '-') {
        noise = -noise;
    }
    const auto isNoiseAddition = (line[index + 3] == '^');
    // If the noise is "direct", -1 means 0x1f.
    if (!isNoiseAddition && (noise < 0)) {
        noise = 0x20 + noise;
    }
    index += 5;

    // Volume.
    const auto volume = line.substring(index, index + 1).getHexValue32();
    ++index;
    const auto isVolumeIncrease = (line[index] == '+');
    const auto isVolumeDecrease = (line[index] == '-');

    foundIncDecOutput = (isVolumeIncrease || isVolumeDecrease || isNoiseAddition || isShiftPeriodAddition);

    return buildInstrumentCell(isTone, isNoise, shiftPeriod, isShiftPeriodAddition, noise, isNoiseAddition, volume, isVolumeIncrease, isVolumeDecrease);
}

PsgInstrumentCell Vt2InstrumentBuilder::buildInstrumentCell(bool isTone, bool isNoise, int shiftPeriod, bool isShiftPeriodAddition,
        int noise, bool isNoiseAddition,
        int volume, bool isVolumeIncrease, bool isVolumeDecrease) noexcept
{
    const auto link = isTone ? PsgInstrumentCellLink::softOnly : PsgInstrumentCellLink::noSoftNoHard;

    // Behavior for the pitch: if addition, use the current shift pitch as a value, else use the given value plus the shift.
    int softwarePitchToPlay;    // NOLINT(*-init-variables)
    if (isShiftPeriodAddition) {
        softwarePitchToPlay = currentShiftPeriod;
        currentShiftPeriod += shiftPeriod;
    } else {
        softwarePitchToPlay = shiftPeriod + currentShiftPeriod;
    }
    softwarePitchToPlay = NumberUtil::correctNumber(softwarePitchToPlay, PsgValues::minimumPitch, PsgValues::maximumPitch);

    // Behavior for the noise: if addition, use the current shift noise as a value, else use the given value plus the shift.
    int noiseToPlay;    // NOLINT(*-init-variables)
    if (isNoiseAddition) {
        noiseToPlay = currentShiftNoise;
        currentShiftNoise += noise;
    } else {
        noiseToPlay = noise + currentShiftNoise;
    }
    noiseToPlay = static_cast<int>(static_cast<unsigned int>(noiseToPlay) & 0b11111U);
    // If the noise is 0, use 1 so that we are sure it will be played.
    if (noiseToPlay == 0) {
        ++noiseToPlay;
    }

    // The volume is the one read in the line, plus the shift.
    auto currentVolume = NumberUtil::correctNumber(volume + currentShiftVolume, 0, 15);
    // Only AFTER the volume is set that the shift volume is modified.
    if (isVolumeIncrease) {
        currentShiftVolume += 1;
    } else if (isVolumeDecrease) {
        currentShiftVolume -= 1;
    }

    // A hardware curve of 8 is used, this is to help the comparison for tests, because it is automatically corrected to minimum 8.
    // The pitch is inverted because on AT2, it is inverted.
    return { link, currentVolume, isNoise ? noiseToPlay : 0, 0, 0, 0,
             -softwarePitchToPlay, 4, 0, 0, 0, 0, 8, false,
             PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
             PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    };
}

std::pair<int, bool> Vt2InstrumentBuilder::findLoopToIndex() const noexcept
{
    constexpr auto loopCharIndex = 18;

    auto loopToIndex = 0;
    auto lineIndex = 0;
    // Is there a "L" at the end?
    for (const auto& line : lines) {
        if ((line.length() > loopCharIndex) && (line[loopCharIndex] == 'L')) {
            // Founds the loop.
            loopToIndex = lineIndex;
            return { loopToIndex, true };
        }

        ++lineIndex;
    }

    // No loop.
    return { 0, false };
}

bool Vt2InstrumentBuilder::areCellsDifferent(const std::vector<PsgInstrumentCell>& cells1, const std::vector<PsgInstrumentCell>& cells2) noexcept
{
    // Not the same size? Not the same, then!
    if (cells1.size() != cells2.size()) {
        return true;
    }

    // Compares each Cell.
    for (auto iterator1 = cells1.cbegin(), iterator2 = cells2.cbegin(); iterator1 != cells1.cend(); ++iterator1, ++iterator2) {
        if (*iterator1 != *iterator2) {
            // Different. We can leave directly.
            return true;
        }
    }

    // Identical.
    return false;
}

void Vt2InstrumentBuilder::encodeInstrumentCell(const PsgInstrumentCell& cell) const noexcept
{
    songBuilder.startNewPsgInstrumentCell();
    songBuilder.setCurrentPsgInstrumentCell(cell);
    songBuilder.finishCurrentPsgInstrumentCell();
}

}   // namespace arkostracker
