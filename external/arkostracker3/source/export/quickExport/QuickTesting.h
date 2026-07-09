#pragma once

#include "../PlayerType.h"
#include "../../ui/utils/backgroundTask/BackgroundTaskWithProgress.h"

namespace arkostracker
{

class MainController;
class Song;

/** Exports the song into a DSK or SNA, with a tester and a player. */
class QuickTesting final : public Task<std::unique_ptr<bool>>
{
public:

    /**
     * Constructor.
     * @param song the Song.
     * @param toDsk true to export as DSK, false for SNA.
     * @param playerType the type of the player to use.
     * @param outputFile the file to create.
     */
    QuickTesting(std::shared_ptr<Song> song, bool toDsk, PlayerType playerType, juce::File outputFile) noexcept;

    // Task method implementations.
    // ===================================================
    std::pair<bool, std::unique_ptr<bool>> performTask() noexcept override;

private:
    std::shared_ptr<Song> song;
    bool toDsk;
    PlayerType playerType;
    juce::File outputFile;
};

}   // namespace arkostracker
