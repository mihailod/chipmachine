#include "InstrumentStreamExporter.h"

#include "../../../business/song/tool/optimizers/InstrumentOptimizer.h"
#include "../../../player/PsgRegistersConverter.h"

namespace arkostracker 
{

std::unique_ptr<Instrument> InstrumentStreamExporter::exportStreamAsInstrument(const juce::String& name, const int channelIndex, const std::vector<PsgRegisters>& psgRegisterFrames,
                                                                               const bool isLooping, const bool encodeAsForcedPeriods, const int sourcePsgFrequencyHz,
                                                                               const int targetPsgFrequencyHz, const float referenceFrequencyHz) noexcept
{
    auto psgPart = PsgPart();

    // Converts the registers to the target PSG frequency.
    PsgRegistersConverter converter(sourcePsgFrequencyHz, targetPsgFrequencyHz, referenceFrequencyHz);
    auto psgCells = converter.encodeAsInstrumentCells(psgRegisterFrames, channelIndex, encodeAsForcedPeriods);
    if (psgCells.empty()) {
        jassertfalse;           // Nothing to encode? Let's avoid a crash!
        psgCells.emplace_back();
    }

    for (const auto& cell : psgCells) {
        psgPart.addCell(cell);
    }
    psgPart.setMainLoop(Loop(0, static_cast<int>(psgCells.size() - 1), isLooping));

    auto generatedInstrument = Instrument::buildPsgInstrument(name, psgPart);

    // Optimizes the instrument (cleaner!).
    InstrumentOptimizer instrumentOptimizer(*generatedInstrument, false, true);
    instrumentOptimizer.optimize();

    return generatedInstrument;
}

}   // namespace arkostracker
