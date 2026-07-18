#pragma once

#include <memory>

#include "../ExportConfiguration.h"
#include "../SongExportResult.h"
#include "../../song/instrument/InstrumentType.h"
#include "../../utils/task/Task.h"

namespace arkostracker
{

class Song;
class SourceGenerator;

/**
 * Export to Raw Linear format, a non-optimized format containing a simple fully linear format with
 * wait/noise-instrument-channel/end. Useful to export sample notes.
 *
 * NOTE: Legato is not supported because the instrument comes from the player, to which the instrument
 * is always known. Supporting it would require another way... See if anyone is interested first!
 */
class RawLinearExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param song the Song. No need to optimize it.
     * @param exportConfiguration data about how to export.
     * @param instrumentTypes the instrument types to export.
     */
    RawLinearExporter(const std::shared_ptr<Song>& song, ExportConfiguration exportConfiguration,
        std::set<InstrumentType> instrumentTypes) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    static constexpr auto dataTypeEndFrame = 255;
    static constexpr auto dataTypeWait = 254;
    static constexpr auto dataTypeEnd = 253;
    static constexpr auto absentNoteValue = 255;
    static constexpr auto absentInstrumentValue = 255;
    static constexpr auto endEffect = 0;

    /** @return the base label to be used before every label. */
    juce::String getBaseLabel() const noexcept;

    /** Generates all the data of the song. The header is already encoded. */
    void generateData(const Id& subsongId, SourceGenerator& sourceGenerator) noexcept;

    /** Encodes the wait, if present. If encoding, the wait counter is reset. */
    static void encodeWait(SourceGenerator& sourceGenerator, int& waitCounter) noexcept;

    /** Builds the instrument IDs that can be exported, and the RST Instrument ID. */
    void prepare() noexcept;

    /**
     * @return true if the given Instrument ID is valid.
     * @param instrumentId the Instrument ID.
     */
    bool isInstrumentValid(const Id& instrumentId) const noexcept;

    std::shared_ptr<Song> song;
    ExportConfiguration exportConfiguration;
    std::set<InstrumentType> instrumentTypes;

    Id rstInstrumentId;
    std::set<int> channelWithValidInstrumentType;
    std::set<Id> validInstrumentIds;
};

}   // namespace arkostracker
