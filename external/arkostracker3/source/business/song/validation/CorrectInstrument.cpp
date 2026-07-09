#include "CorrectInstrument.h"

#include <algorithm>

#include "../../../song/cells/CellConstants.h"
#include "../../../song/instrument/Instrument.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "CheckLoopStartEnd.h"

namespace arkostracker 
{

void CorrectInstrument::correctInstrument(Instrument& instrument) noexcept
{
    // Forces the alpha, if not opaque.
    const auto color = instrument.getArgbColor();
    if ((color & 0xff000000) != 0xff000000) {
        instrument.setColor(color | 0xff000000);
    }

    switch (instrument.getType()) {
        case InstrumentType::psgInstrument:
            correctPsgInstrument(instrument);
            break;
        case InstrumentType::sampleInstrument:
            correctSampleInstrument(instrument);
            break;
        default:
            jassertfalse;       // Should never happen!
            break;
    }
}

void CorrectInstrument::correctPsgInstrument(Instrument& instrument) noexcept
{
    jassert(instrument.getType() == InstrumentType::psgInstrument);

    // If empty, one entry must be created.
    auto& psgPart = instrument.getPsgPart();
    if (psgPart.getLength() == 0) {
        psgPart.addCell(PsgInstrumentCell::getEmptyPsgInstrumentCell());
    }

    // Corrects the speed.
    const auto speed = psgPart.getSpeed();
    psgPart.setSpeed(NumberUtil::correctNumber(speed, PsgValues::minimumSpeed, PsgValues::maximumSpeed));

    // Checks the start/end.
    const auto length = psgPart.getLength();
    jassert(length > 0);        // Corrected before!

    // Correcting (overwriting) the loops. First the main loop.
    psgPart.setMainLoop(CheckLoopStartEnd::checkLoopStartEnd(psgPart.getMainLoop(), length));
    // Then the auto-spreads.
    std::unordered_map<PsgSection, Loop> newAutoSpreadLoops;
    for (const auto& [psgSection, loop] : psgPart.getAutoSpreadAllLoops()) {
        const auto newLoop = CheckLoopStartEnd::checkLoopStartEnd(loop, length);
        newAutoSpreadLoops[psgSection] = newLoop;
    }
    psgPart.setAutoSpreadLoops(newAutoSpreadLoops);

    // Checks every Cell. They are actually all rewritten.
    for (auto cellIndex = 0; cellIndex < length; ++cellIndex) {
        const auto& cell = psgPart.getCellRefConst(cellIndex);
        const auto link = cell.getLink();
        const auto retrig = cell.isRetrig();
        const auto noise = NumberUtil::correctNumber(cell.getNoise(), PsgValues::minimumNoise, PsgValues::maximumNoise);
        const auto volume = NumberUtil::correctNumber(cell.getVolume(), PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);
        const auto primaryPeriod = NumberUtil::correctNumber(cell.getPrimaryPeriod(), PsgValues::minimumPeriod,
            PsgInstrumentCellLinkHelper::isPrimarySoftware(link) ? PsgValues::maximumSoftwarePeriod : PsgValues::maximumHardwarePeriod);
        const auto primaryArpeggioNoteInOctave = NumberUtil::correctNumber(cell.getPrimaryArpeggioNoteInOctave(), CellConstants::firstNoteInOctave,
                CellConstants::lastNoteInOctave);
        const auto primaryArpeggioOctave = NumberUtil::correctNumber(cell.getPrimaryArpeggioOctave(), PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave);
        const auto primaryPitch = NumberUtil::correctNumber(cell.getPrimaryPitch(), PsgValues::minimumPitch, PsgValues::maximumPitch);
        const auto ratio = NumberUtil::correctNumber(cell.getRatio(), PsgValues::minimumRatio, PsgValues::maximumRatio);
        const auto hardwareEnvelope = NumberUtil::correctNumber(cell.getHardwareEnvelope(), PsgValues::minimumHardwareEnvelope, PsgValues::maximumHardwareEnvelope);
        const auto secondaryPeriod = NumberUtil::correctNumber(cell.getSecondaryPeriod(), PsgValues::minimumPeriod,
            PsgInstrumentCellLinkHelper::isPrimarySoftware(link) ? PsgValues::maximumHardwarePeriod : PsgValues::maximumSoftwarePeriod);
        const auto secondaryArpeggioNoteInOctave = NumberUtil::correctNumber(cell.getSecondaryArpeggioNoteInOctave(), CellConstants::firstNoteInOctave,
                                                                           CellConstants::lastNoteInOctave);
        const auto secondaryArpeggioOctave = NumberUtil::correctNumber(cell.getSecondaryArpeggioOctave(), PsgValues::minimumArpeggioOctave, PsgValues::maximumArpeggioOctave);
        const auto secondaryPitch = NumberUtil::correctNumber(cell.getSecondaryPitch(), PsgValues::minimumPitch, PsgValues::maximumPitch);

        const auto isSidActivated = cell.isSidActivated();
        const auto sidCycleBalancePercent = NumberUtil::correctNumber(cell.getSidBalancePercent(), PsgValues::minimumPercent, PsgValues::maximumPercent);
        const auto sidLowShelfVolume = NumberUtil::correctNumber(cell.getSidLowShelfVolume(), PsgValues::minimumVolume, PsgValues::maximumVolumeNoHard);
        const auto sidRatio = NumberUtil::correctNumber(cell.getSidRatio(), PsgValues::sidMinimumRatio, PsgValues::sidMaximumRatio);
        const auto sidArpeggio = NumberUtil::correctNumber(cell.getSidArpeggio(), PsgValues::sidMinimumArpeggio, PsgValues::sidMaximumArpeggio);
        const auto sidPitch = NumberUtil::correctNumber(cell.getSidPitch(), PsgValues::sidMinimumPitch, PsgValues::sidMaximumPitch);

        auto newCell = PsgInstrumentCell(link, volume, noise, primaryPeriod, primaryArpeggioNoteInOctave, primaryArpeggioOctave, primaryPitch, ratio,
                                         secondaryPeriod, secondaryArpeggioNoteInOctave, secondaryArpeggioOctave, secondaryPitch, hardwareEnvelope, retrig,
                                         isSidActivated, sidCycleBalancePercent, sidLowShelfVolume, sidRatio, sidArpeggio, sidPitch);

        psgPart.setCell(cellIndex, newCell);
    }
}

void CorrectInstrument::correctSampleInstrument(Instrument& instrument) noexcept
{
    jassert(instrument.getType() == InstrumentType::sampleInstrument);

    auto& samplePart = instrument.getSamplePart();
    const auto& sample = samplePart.getSample();
    const auto amplificationRatio = NumberUtil::correctNumber(samplePart.getAmplificationRatio(), PsgValues::minimumSampleAmplificationRatio,
            PsgValues::maximumSampleAmplificationRatio);
    const auto length = sample->getLength();
    jassert(length > 0);            // Empty sample?

    const auto loop = samplePart.getLoop();
    auto end = NumberUtil::correctNumber(loop.getEndIndex(), 0, length - 1);
    end = std::max(end, 0);

    const auto start = NumberUtil::correctNumber(loop.getStartIndex(), 0, end);
    const auto frequencyHz = NumberUtil::correctNumber(samplePart.getFrequencyHz(), PsgFrequency::minimumSampleFrequency, PsgFrequency::maximumSampleFrequency);
    const auto diginote = NumberUtil::correctNumber(samplePart.getDigidrumNote(), 0, CellConstants::maximumNote);

    samplePart.setLoop(Loop(start, end, loop.isLooping()));
    samplePart.setAmplificationRatio(amplificationRatio);
    samplePart.setFrequencyHz(frequencyHz);
    samplePart.setDigidrumNote(diginote);
}

}   // namespace arkostracker
