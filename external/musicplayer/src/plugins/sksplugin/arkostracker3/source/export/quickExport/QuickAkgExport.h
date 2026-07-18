#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"
#include "../../utils/task/Task.h"

namespace arkostracker
{

class MainController;
class Song;

/** Generates the player code. */
class QuickAkgPlayerExport final : public Task<std::unique_ptr<juce::MemoryBlock>>
{
public:

    /**
     * Constructor.
     * @param address where to compile.
     * @param withSfx true to also use sfxs.
     */
    QuickAkgPlayerExport(int address, bool withSfx) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<juce::MemoryBlock>> performTask() noexcept override;

private:
    int address;
    bool withSfx;
};

/** Generates the SFX code from a Song. */
class QuickSfxExport final : public Task<std::unique_ptr<juce::MemoryBlock>>
{
public:

    /**
     * Constructor.
     * @param song the SFX Song.
     * @param address where to compile.
     */
    QuickSfxExport(const Song& song, int address) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<juce::MemoryBlock>> performTask() noexcept override;

private:
    const Song& song;
    int address;
};

/** Loads and generates the SFX code. */
class QuickLoadAndCompileSfxExport final : public Task<std::unique_ptr<juce::MemoryBlock>>
{
public:

    /**
     * Constructor.
     * @param file the file to load. It should exist.
     * @param address where to compile.
     */
    QuickLoadAndCompileSfxExport(const juce::File& file, int address) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<juce::MemoryBlock>> performTask() noexcept override;

private:
    const juce::File& file;
    int address;
};

/** Generates the whole DSK. */
class QuickExport final : public Task<std::unique_ptr<bool>>
{
public:

    /**
     * Constructor. All the values must be valid.
     * @param outputDskFile the DSK to create.
     * @param playerAddress the player address.
     * @param musicAddress the music address.
     * @param song the song (not the SFX).
     * @param sfxAddress the sfx address, if SFX a used.
     * @param sfxFile the sfx File to load, if SFX a used.
     */
    QuickExport(juce::File outputDskFile, int playerAddress, int musicAddress, std::shared_ptr<const Song> song,
        OptionalInt sfxAddress, juce::File sfxFile) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    juce::File outputDskFile;
    int playerAddress;
    int musicAddress;
    std::shared_ptr<const Song> song;
    OptionalInt sfxAddress;
    juce::File sfxFile;
};

}       // namespace arkostracker
