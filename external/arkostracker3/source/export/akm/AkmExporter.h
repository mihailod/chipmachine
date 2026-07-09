#pragma once

#include <memory>

#include "../SongExportResult.h"
#include "../../utils/task/Task.h"
#include "../../song/Song.h"
#include "../ExportConfiguration.h"

namespace arkostracker 
{

/**
 * Exports a Song to AKG format, as source.
 * The input Song will be optimized, nothing to do before.
 */
class AkmExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:

    /**
     * Constructor.
     * @param originalSong the original Song. No need to optimize it.
     * @param exportConfiguration data about how to export.
     */
    AkmExporter(const Song& originalSong, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:
    /**
     * @return the base label to be used before every label.
     * @param subsongIndex if >=0, adds the subsong index after it.
    */
    juce::String getBaseLabel(int subsongIndex = -1) const noexcept;

    const Song& originalSong;
    std::unique_ptr<Song> song;
    ExportConfiguration exportConfiguration;
};

}   // namespace arkostracker

