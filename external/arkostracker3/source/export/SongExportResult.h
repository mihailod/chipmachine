#pragma once

#include <juce_core/juce_core.h>

#include "playerConfiguration/PlayerConfiguration.h"
#include "../utils/ErrorReport.h"

namespace arkostracker 
{

/**
 * The generic result of an export for a Song with Subsongs.
 *
 * The PlayerConfiguration can be set directly, or you can use the addFlag method directly to populate the one
 * embedded to the current object.
 */
class SongExportResult
{
public:
    /** Constructor. */
    SongExportResult() noexcept;

    /**
     * Convenience constructor with some data.
     * @param songOutputStream the data of the Song. No Subsong data is added. Useful for players without any Subsongs, but they can be added later.
     * @param playerConfiguration the Player Configuration.
     */
    SongExportResult(std::unique_ptr<juce::MemoryOutputStream> songOutputStream, PlayerConfiguration playerConfiguration) noexcept;

    /**
     * Sets a Song stream (as opposite to the Subsong one).
     * @param outputStream the Subsong stream.
     */
    void setSongStream(std::unique_ptr<juce::MemoryOutputStream> outputStream) noexcept;

    /** Sets the player configuration. */
    void setPlayerConfiguration(const PlayerConfiguration& playerConfiguration) noexcept;

    /**
     * Adds a Subsong stream.
     * @param outputStream the Subsong stream.
     */
    void addSubsongStream(std::unique_ptr<juce::MemoryOutputStream> outputStream) noexcept;

    /**
     * Adds a warning line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addWarning(const juce::String& text, OptionalInt lineNumber = { }) noexcept;

    /**
     * Adds an error line.
     * @param text a human-readable string.
     * @param lineNumber the line number where the report is about, if any.
     */
    void addError(const juce::String& text, OptionalInt lineNumber = { }) noexcept;

    /** @return true if there are no critical error. */
    bool isOk() const noexcept;

    /** @return a reference to the error report. */
    const ErrorReport& getErrorReportRef() noexcept;

    /** @return a reference to the Player Configuration. */
    const PlayerConfiguration& getPlayerConfigurationRef() noexcept;

    /**
     * Adds a flag. It may already be here. This may trigger the adding of other flags.
     * @param flag the flag.
     */
    void addFlag(PlayerConfigurationFlag flag) noexcept;

    /**
     * Adds a flag, if wanted (useful to avoid IFs). It may already be here. This may trigger the adding of other flags.
     * @param addFlag true to add the flag.
     * @param flag the flag.
     */
    void addFlag(bool addFlag, PlayerConfigurationFlag flag) noexcept;

    /** @return a copy of the data of the Song Output Stream. */
    juce::MemoryBlock getSongData() const noexcept;

    /** @return a copy of the data of the Subsong Output Streams. */
    std::vector<juce::MemoryBlock> getSubsongData() const noexcept;

    /** @return a copy of the data of the Song plus the Subsong Output Stream*s*. */
    juce::MemoryBlock getAggregatedData() const noexcept;

private:
    std::unique_ptr<juce::MemoryOutputStream> songOutputStream;
    std::vector<std::unique_ptr<juce::MemoryOutputStream>> subsongOutputStreams;
    PlayerConfiguration playerConfiguration;
    ErrorReport errorReport;
};

}   // namespace arkostracker

