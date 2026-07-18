#include "InstrumentCounter.h"

#include "../../../song/Song.h"
#include "browser/CellBrowser.h"
#include "browser/SpecialCellBrowser.h"

namespace arkostracker
{

std::vector<InstrumentCounter::InstrumentResult> InstrumentCounter::countInstruments(const Song& song) noexcept
{
    std::vector<InstrumentResult> results;

    const auto instrumentCount = song.getInstrumentCount();

    // Browses the Positions and browses their Cells.
    for (auto instrumentIndex = 1; instrumentIndex < instrumentCount; ++instrumentIndex) {      // Instrument 0 is skipped.

        auto instrumentOccurrence = 0;
        constexpr auto stopAtLoopEnd = false;
        CellBrowser::browseConstSongCells(song, stopAtLoopEnd, [&](const Cell& cell) {
            if ((cell.isNoteAndInstrument() && (cell.getInstrument() == instrumentIndex))) {
                ++instrumentOccurrence;
            }
            return false;
        });

        // Sample instrument? If yes, also checks the Event Tracks.
        const auto instrumentIdOptional = song.getInstrumentId(instrumentIndex);
        if (instrumentIdOptional.isAbsent()) {
            jassertfalse;      // Should never happen!
            return { };
        }
        const auto instrumentId = instrumentIdOptional.getValue();

        auto isSample = false;
        juce::String name;
        song.performOnConstInstrument(instrumentId, [&](const Instrument& instrument) {
            isSample = (instrument.getType() == InstrumentType::sampleInstrument);
            name = instrument.getName();
        });
        if (isSample) {
            SpecialCellBrowser::browseConstSongSpecialCells(song, false, stopAtLoopEnd, [&](const SpecialCell& specialCell) {
                if (specialCell.getValue() == instrumentIndex) {
                    ++instrumentOccurrence;
                }
                return false;
            });
        }

        results.emplace_back(instrumentId, name, instrumentIndex, instrumentOccurrence, isSample);
    }

    return results;
}

} // namespace arkostracker
