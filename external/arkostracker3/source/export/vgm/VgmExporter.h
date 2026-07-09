#pragma once

#include <juce_core/juce_core.h>

#include "../../song/Song.h"
#include "../../utils/task/Task.h"

namespace arkostracker 
{

class PsgRegisters;

/** Exports as VGM, with a maximum of 2 PSGs. */
class VgmExporter final : public Task<std::unique_ptr<juce::MemoryOutputStream>>
{
public:
    static const int maximumPsgCount;

    enum class PreliminaryCheck : unsigned char
    {
        ok,
        tooManyPsgs,
        differentPsgFrequencies,
    };

    /**
     * Constructor.
     * @param song the Song to read.
     * @param subsongId the ID of the Subsong to read. Must be valid.
     * @param compress true to compress to zip (vgz instead of vgm).
     */
    VgmExporter(std::shared_ptr<Song> song, Id subsongId, bool compress) noexcept;

    /**
     * @return a preliminary check to know if a problem is present in the Song. A text to display is also returned.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to export.
     */
    static std::pair<PreliminaryCheck, juce::String> performPreliminaryCheck(const Song& song, const Id& subsongId) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<juce::MemoryOutputStream>> performTask() noexcept override;

private:

    /**
     * Encodes the given PSG registers to the given output stream.
     * @param vgmData the stream to write to.
     * @param psgRegisters the registers to encode.
     * @param previousHardwareEnvelope the previous Hardware envelope (0-15). Useful to know if the envelope PSG Register must be encoded because of a change, or not.
     * @param isSecondPsg true if second PSG, false if first.
     */
    static void encodePsgFrame(juce::MemoryOutputStream& vgmData, const PsgRegisters& psgRegisters, int previousHardwareEnvelope, bool isSecondPsg) noexcept;

    /**
     * Writes a text for GD3.
     * @param vgmData where to write.
     * @param text the text.
     * @param addEmptyNonEnglishSentence true to add an empty sentence next, for non-english version.
     */
    static void writeGd3Text(juce::MemoryOutputStream& vgmData, const juce::String& text, bool addEmptyNonEnglishSentence = true) noexcept;

    /**
     * Writes a 32 little-endian int at a specific location. WARNING! This changes the position of the given stream!
     * @param vgmData where to write.
     * @param whereToWrite where to write.
     * @param data what to write.
     */
    static void writeInt(juce::MemoryOutputStream& vgmData, juce::int64 whereToWrite, int data) noexcept;

    /**
     * Encodes the Wait. May be optimized according to the wait value.
     * @param vgmData where to write.
     * @param waitValue the wait value.
     */
    static void encodeWait(juce::MemoryOutputStream& vgmData, int waitValue) noexcept;

    /** @return the VGM data (uncompressed). */
    std::unique_ptr<juce::MemoryOutputStream> exportVgm() noexcept;

    const std::shared_ptr<Song> song;                   // The Song.
    const Id subsongId;                                 // The id of the Subsong to read. Must be valid.
    bool compress;
    std::vector<int> previousHardwareEnvelopes;         // One for each PSG.
};

}   // namespace arkostracker
