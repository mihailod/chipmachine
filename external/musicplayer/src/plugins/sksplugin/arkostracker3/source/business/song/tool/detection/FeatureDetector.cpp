#include "FeatureDetector.h"

#include "../../../../song/Song.h"
#include "../InstrumentCounter.h"
#include "../browser/CellBrowser.h"
#include "../browser/SpecialCellBrowser.h"

namespace arkostracker
{

FeatureDetector::Result FeatureDetector::perform(const Song& song) noexcept
{
    auto areDigidrumsUsed = false;
    auto areSidUsed = determineAreSidUsed(song);
    auto maximumChannelCount = PsgValues::channelCountPerPsg;
    auto arePsgInstrumentsUsed = false;
    auto areSampleInstrumentsUsed = false;

    const auto subsongIds = song.getSubsongIds();
    for (const auto& subsongId : subsongIds) {
        if (!areDigidrumsUsed) {
            areDigidrumsUsed = determineAreDigidrumsUsed(song, subsongId);
        }
        if (!arePsgInstrumentsUsed && !areSampleInstrumentsUsed) {
            const auto [localArePsgInstrumentsUsed, localAreSampleInstrumentsUsed] = determineArePsgAndSamplesUsed(song);
            arePsgInstrumentsUsed |= localArePsgInstrumentsUsed;
            areSampleInstrumentsUsed |= localAreSampleInstrumentsUsed;
        }

        maximumChannelCount = std::max(maximumChannelCount, song.getChannelCount(subsongId));
    }

    return { areDigidrumsUsed, areSidUsed, arePsgInstrumentsUsed,
        areSampleInstrumentsUsed, maximumChannelCount };
}

bool FeatureDetector::determineAreDigidrumsUsed(const Song& song, const Id& subsongId) noexcept
{
    auto events = std::unordered_set<int>();
    const auto instrumentCount = song.getInstrumentCount();

    // Gets all the events, if within the instrument count.
    SpecialCellBrowser::browseConstSubsongSpecialCells(song, subsongId, false, true, [&] (const SpecialCell& cell) {
        if (!cell.isEmpty()) {
            if (const auto event = cell.getValue(); event < instrumentCount) {
                events.insert(event);
            }
        }
    });

    auto areDigidrumsUsed = false;
    // As soon as an event related a sample, consider a digidrum is used.
    for (const auto event : events) {
        song.performOnConstInstrumentFromIndex(event, [&] (const Instrument& instrument) {
            if (instrument.getType() == InstrumentType::sampleInstrument) {
                areDigidrumsUsed = true;
            }
        });

        if (areDigidrumsUsed) {     // As soon as one digidrum is used, we can stop.
            break;
        }
    }

    return areDigidrumsUsed;
}

bool FeatureDetector::determineAreSidUsed(const Song& song) noexcept
{
    auto areSidUsed = false;

    const auto instrumentCountResult = InstrumentCounter::countInstruments(song);

    for (const auto& instrumentResult : instrumentCountResult) {
        if ((instrumentResult.getCount() > 0) && !instrumentResult.isSample()) {
            const auto instrumentId = instrumentResult.getId();

            song.performOnConstPsgInstrument(instrumentId, [&] (const Instrument& /*instrument*/, const PsgPart& psgPart) {
                const auto endIndex = psgPart.getMainLoopRef().getEndIndex();
                for (auto cellIndex = 0; !areSidUsed && (cellIndex <= endIndex); ++cellIndex) {
                    if (const auto& cell = psgPart.getCellRefConst(cellIndex); cell.isSidActivated()) {
                        areSidUsed = true;
                    }
                }
            });
        }
    }

    return areSidUsed;
}

std::pair<bool, bool> FeatureDetector::determineArePsgAndSamplesUsed(const Song& song) noexcept
{
    auto useInstruments = false;
    auto useSamples = false;

    std::unordered_set<int> browsedInstrumentIndexes;

    // One first pass to gather used instrument indexes.
    CellBrowser::browseConstSongCells(song, true, [&] (const Cell& cell) {
        if (cell.isNoteAndInstrument()) {
            const auto instrumentIndex = cell.getInstrument().getValue();
            if (browsedInstrumentIndexes.find(instrumentIndex) == browsedInstrumentIndexes.cend()) {
                browsedInstrumentIndexes.insert(instrumentIndex);
            }
        }
        return false;
    });

    // Second pass to get the types.
    for (const auto instrumentIndex : browsedInstrumentIndexes) {
        if (instrumentIndex == 0) {     // Ignore the 0th instrument.
            continue;
        }
        // We can stop as soon as both instrument types are found.
        if (useInstruments && useSamples) {
            break;
        }

        song.performOnConstInstrumentFromIndex(instrumentIndex, [&] (const Instrument& instrument) {
            switch (instrument.getType()) {
                case InstrumentType::psgInstrument:
                    useInstruments = true;
                    break;
                case InstrumentType::sampleInstrument:
                    useSamples = true;
                    break;
            }
        });
    }

    return { useInstruments, useSamples, };
}

}   // namespace arkostracker