#include "GeneratePeriods.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../export/sourceGenerator/SourceGenerator.h"
#include "../../player/PsgPeriod.h"
#include "../../song/cells/CellConstants.h"
#include "../../utils/NoteUtil.h"

namespace arkostracker
{

std::unique_ptr<juce::MemoryOutputStream> GeneratePeriods::generate(const int psgFrequencyHz, const float referenceFrequencyHz, const SourceProfile& sourceProfile) noexcept
{
    auto outputStream = std::make_unique<juce::MemoryOutputStream>(1024);
    SourceGenerator sourceGenerator(sourceProfile.getSourceGeneratorConfiguration(), *outputStream);

    for (auto noteIndex = CellConstants::minimumNote; noteIndex <= CellConstants::maximumNote; ++noteIndex) {
        const auto period = PsgPeriod::getPeriod(psgFrequencyHz, referenceFrequencyHz, noteIndex);

        const auto noteInOctave = NoteUtil::getNoteInOctave(noteIndex);
        const auto octave = NoteUtil::getOctaveFromNote(noteIndex);

        sourceGenerator.declareWord(period, "Note index: " + juce::String(noteIndex)
            + " (note in octave: " + juce::String(noteInOctave) + ", octave: " + juce::String(octave) + ")");
    }

    sourceGenerator.declareEndOfFile();

    return outputStream;
}

}   // namespace arkostracker
