#pragma once

#include <memory>

#include <juce_core/juce_core.h>

#include "../../song/Song.h"
#include "../../utils/task/Task.h"
#include "YmStreamEncoder.h"

namespace arkostracker 
{

/**
 * Class that generates a YM file for the Subsong of a Song.
 * Even in "non-interleaved" mode, the YM can be generated linearly (using 14 passes (!)), so no need to store anything in memory.
 */
class YmExporter final : public Task<std::unique_ptr<juce::MemoryOutputStream>>
{
public:
    /**
     * Constructor.
     * @param song the Song to read.
     * @param subsongId the ID of the Subsong to read. Must be valid.
     * @param psgIndex the index of the unique PSG to get the data from. Must be valid.
     * @param interleaved true if the YM format is generated as "interleaved". Ignored if YM3.
     * @param isYm3 true if YM3, false if YM6. Use false in unsure.
     * @param targetPsgFrequency the possible target PSG frequency.
     */
    YmExporter(std::shared_ptr<Song> song, Id subsongId, int psgIndex, bool interleaved, bool isYm3, OptionalInt targetPsgFrequency = OptionalInt()) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<juce::MemoryOutputStream>> performTask() noexcept override;

private:
    const std::shared_ptr<Song> song;                   // The Song.
    const Id subsongId;                                 // The id of the Subsong to read. Must be valid.
    const int psgIndex;                                 // The index of the unique PSG to get the data from.
    const bool interleaved;                             // True if the YM format is generated as "interleaved".
    const bool isYm3;
    OptionalInt targetPsgFrequency;
    const int digiChannel;

    std::unique_ptr<YmStreamEncoder> streamEncoder;     // The stream encoder to YM, different if interleaved or not.
};

}   // namespace arkostracker
