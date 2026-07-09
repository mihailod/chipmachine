#pragma once

#include "../../business/song/tool/InstrumentCounter.h"
#include "../../utils/task/Task.h"
#include "../ExportConfiguration.h"
#include "../SongExportResult.h"

namespace arkostracker
{

class Song;

/** Class to generate exported samples. */
class SampleExporter final : public Task<std::unique_ptr<SongExportResult>>
{
public:
    /**
     * Constructor.
     * @param song the song.
     * @param exportConfiguration how to export.
     * @param pitch a possible pitch to apply to all samples, in semi-tones.
     */
    SampleExporter(const Song& song, ExportConfiguration exportConfiguration, double pitch) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

private:

    /**
     * @return the base label to be used before every label.
     * @param addEndSeparator true to add a "_" at the end.
     */
    juce::String getSampleBaseLabel(bool addEndSeparator = false) const noexcept;

    /**
     * @return the label used for a sample.
     * @param sampleIndex the index. Should be >0.
     */
    juce::String getSampleLabel(int sampleIndex) const noexcept;

    bool mustEncodeSample(const InstrumentCounter::InstrumentResult& entry) const noexcept;

    const Song& song;
    ExportConfiguration exportConfiguration;
    double pitch;
};

}   // namespace arkostracker
