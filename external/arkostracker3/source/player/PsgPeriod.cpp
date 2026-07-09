#include "PsgPeriod.h"

#include "../song/cells/CellConstants.h"
#include "../utils/NumberUtil.h"

namespace arkostracker 
{

PsgPeriod::PsgPeriod(const int psgFrequencyHz, const float referenceFrequencyHz) noexcept :
        periodToNoteIndex(),
        noteIndexToPeriod()
{
    // Builds double maps.
    for (auto noteIndex = 0; noteIndex <= CellConstants::maximumNote; ++noteIndex) {
        const auto localNoteIndex = noteIndex;      // To silence a warning.
        const auto period = static_cast<juce::uint16>(getPeriod(psgFrequencyHz, referenceFrequencyHz, localNoteIndex));

        periodToNoteIndex.insert(std::make_pair(period, localNoteIndex));
        noteIndexToPeriod.insert(std::make_pair(localNoteIndex, period));
    }
}

int PsgPeriod::getPeriod(const int psgFrequency, const float referenceFrequency, int noteIndex) noexcept
{
    // Note plus arpeggios can go beyond the scope.
    noteIndex = NumberUtil::correctNumber(noteIndex, CellConstants::minimumNote, CellConstants::maximumNote);

    const auto periodDivider = static_cast<double>(psgFrequency) / 8.0;
    static constexpr auto startOctave = -3;

    static constexpr auto notesInOctave = 12;
    const auto octave = noteIndex / notesInOctave + startOctave;
    const auto noteInOctave = (noteIndex % notesInOctave) + 1;

    const auto frequency = static_cast<double>(((referenceFrequency) * pow(2.0, (static_cast<double>(octave) + ((static_cast<double>(noteInOctave) - 10.0) / 12.0)))));
    const auto period = static_cast<int>(round(periodDivider / frequency));

    jassert((period >= 0) && (period <= 0xffff));

    return period;
}

double PsgPeriod::getFrequency(const int psgFrequency, const int period) noexcept
{
    const auto periodDivider = static_cast<double>(psgFrequency) / (8.0 * 2.0);
    return (1.0 / period) * periodDivider;
}

NoteAndShift PsgPeriod::findNoteAndShift(const int period) noexcept
{
    // Finds the closest period, lower bound.
    auto iterator = periodToNoteIndex.lower_bound(static_cast<uint16_t>(period));
    // End reached? It means no higher value has been found. In this case, settles on the last item.
    if (iterator == periodToNoteIndex.end()) {
        --iterator;
    }

    // Exact period? We can stop here.
    const auto foundPeriod = iterator->first;
    auto foundNote = iterator->second;
    if (static_cast<int>(foundPeriod) == period) {
        return { foundNote, 0 };       // No shift, exact value.
    }

    auto foundShift = static_cast<int>(foundPeriod) - period;
    jassert(foundShift > 0);

    // If there is a next period (-- because they are stored from bottom to highest), checks that the diff is not better.
    if (--iterator != periodToNoteIndex.cend()) {
        const auto newFoundShift = period - static_cast<int>(iterator->first);
        if (std::abs(newFoundShift) < std::abs(foundShift)) {
            // The new one is better.
            ++foundNote;
            foundShift = -newFoundShift;    // We go to the note beyond, so the shift is "backwards".
        }
    }

    return { foundNote, foundShift };
}

}   // namespace arkostracker
