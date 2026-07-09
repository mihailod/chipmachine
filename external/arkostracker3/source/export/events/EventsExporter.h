#pragma once

#include <memory>

#include "../../player/SongPlayer.h"
#include "../../utils/task/Task.h"
#include "../ExportConfiguration.h"
#include "../SongExportResult.h"
#include "../sourceGenerator/SourceGenerator.h"

namespace arkostracker
{
class EffectRules;
class Song;
class SourceGeneratorConfiguration;

/**
 * Class that exports all the events from a Song as a source. This is useful to play digidrums, for example.
 */
class EventsExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:
    /** What types to export. */
    enum class Type
    {
        events,
        samples,
    };

    /**
     * Constructor.
     * @param song the Song. No need to optimize it.
     * @param typesToExport what to export. Must not be empty.
     * @param exportConfiguration data about how to export. The unique Subsong to export is within.
     */
    EventsExporter(std::shared_ptr<const Song> song, std::set<Type> typesToExport, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    /**
     * Encodes the header.
     * @param sourceGenerator the source generator.
     */
    void encodeHeader(SourceGenerator& sourceGenerator) const noexcept;

    /**
     * Encodes the given event.
     * If the duration is too large for a 16 bits, it is cut into as many sequences as needed, with events 0.
     * @param sourceGenerator the source generator.
     * @param duration the duration.
     * @param eventNumber the event number. May be 0.
     */
    static void encodeEvent(SourceGenerator& sourceGenerator, unsigned int duration, int eventNumber) noexcept;

    /**
     * Encodes the end event.
     * @param sourceGenerator the source generator.
     */
    void encodeEndEvent(SourceGenerator& sourceGenerator) const noexcept;

    /** Fills the sample indexes. */
    void fillSampleIndexes() noexcept;

    /**
     * @return true if the event must be encoded, according to the types.
     * @param eventNumber the event number.
     */
    bool determineIfEventMustBeEncoded(int eventNumber) const noexcept;

    /** @return the events label. */
    juce::String getEventsLabel() const noexcept;
    /** @return the loop label. */
    juce::String getLoopLabel() const noexcept;

    std::shared_ptr<const Song> song;
    std::set<Type> typesToExport;
    Id subsongId;
    ExportConfiguration exportConfiguration;

    std::unordered_set<int> sampleIndexes;
};

}   // namespace arkostracker
