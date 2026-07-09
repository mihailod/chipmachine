#pragma once

#include "../../player/SongPlayer.h"
#include "../../utils/Id.h"
#include "../../utils/task/Task.h"
#include "../ExportConfiguration.h"
#include "../SongExportResult.h"
#include "../playerConfiguration/PlayerConfiguration.h"
#include "../sourceGenerator/SourceGenerator.h"
#include "process/AkyPatterns.h"
#include "process/RegisterBlockEncoder.h"
#include "process/RegisterBlockLabelProvider.h"
#include "process/RegisterBlockPool.h"

namespace arkostracker 
{

/** Encoder to the AKY format, as a source file. */
class AkyExporter final : public Task<std::unique_ptr<SongExportResult>>,
                          public RegisterBlockLabelProvider
{
public:

    /**
     * Constructor.
     * @param song the Song.
     * @param exportConfiguration how to export.
     */
    AkyExporter(const std::shared_ptr<Song>& song, ExportConfiguration exportConfiguration) noexcept;

    // Task method implementations.
    // ===============================
    std::pair<bool, std::unique_ptr<SongExportResult>> performTask() noexcept override;

    // RegisterBlockLabelProvider method implementations.
    // ===============================================================
    juce::String getLabelForRegisterBlockIndex(int registerBlockId, int lineIndex) noexcept override;

private:
    static const int formatVersion;                                     // The version of the format.

    static const int progressFirst;                                     // A first step of the progress.
    static const int progressSecond;                                    // A second step of the progress.

    /**
     * Plays the song and build Tracks with only registers. There will probably be duplicate!
     * @return the Player Configuration.
     */
    PlayerConfiguration playSongAndBuildRawPatterns() noexcept;
    /** Initializes the given array of ChannelData. */
    static void initializeChannelDataListPerChannel(std::vector<std::unique_ptr<ChannelData>>& channelDataListPerChannel, int channelCount) noexcept;

    /**
     * Fills the given Player Configuration, according to the given Player Results.
     * @param playerConfiguration the player configuration.
     * @param playerResults the frame results.
     */
    static void fillPlayerConfiguration(PlayerConfiguration& playerConfiguration, const std::vector<std::unique_ptr<ChannelPlayerResults>>& playerResults) noexcept;

    /** The RegisterBlocks can surely be optimized by making them loop or having them refer to data of each other. Only the Blocks of the RegisterBlockPool are modified. */
    void referenceOptimizeSortedEncodedBlocks() noexcept;

    /** Encodes the source. */
    void encodeSource() noexcept;

    /** @return the label for the given RegisterBlock ID. */
    juce::String getLabelRegisterBlock(int registerBlockId) const noexcept;

    /**
     * @return the base label to be used before every label.
     * @param addSeparator true to add a separator.
     */
    juce::String getBaseLabel(bool addSeparator = true) const noexcept;

    /** @return the label for a Track which index is given. */
    juce::String getLabelTrack(int trackIndex) const noexcept;

    /** Encodes the possible address change, if one is given. */
    void encodeAddressChange() noexcept;
    /** Encodes the header. */
    void encodeHeader() noexcept;
    /** Encodes the Linker. */
    void encodeLinker() noexcept;

    /** Encodes the Tracks. */
    void encodeTracks() noexcept;
    /** Encodes one Track. */
    void encodeTrack(int trackIndex, const AkyTrack& track) noexcept;

    /** Encodes the used RegisterBlocks. */
    void encodeRegisterBlocks() noexcept;

    /** Declares an address, but that may be relative to the beginning of the Song, if wanted by the user. */
    SourceGenerator& declareAddressOrRelativeAddress(const juce::String& expression) noexcept;

    /**
     * Notifies about the task progress.
     * @param percent the percent.
     */
    void onTaskProgress(int percent) const noexcept;

    SongPlayer songPlayer;                                              // The Song Player.
    std::shared_ptr<Song> song;
    std::unique_ptr<juce::MemoryOutputStream> outputStream;
    ExportConfiguration exportConfiguration;
    SourceGenerator sourceGenerator;
    const Id subsongId;
    const int subsongIndex;
    SidPlayerCapability sidPlayerCapability;

    Location startLocation;
    Location loopLocation;
    Location pastEndLocation;

    int iterationCount;                                                 // How many iterations are in the Song (>0).
    std::unique_ptr<AkyPatterns> patterns;                              // The Patterns.
    RegisterBlockPool registerBlockPool;                                // Pool of all the Register Blocks.
    RegisterBlockEncoder registerBlockEncoder;                          // Encoder of the RegisterBlocks.

    int psgCount;
    int desiredChannelCount;
};

}   // namespace arkostracker
