#pragma once

#include "../loadSong/LoadSong.h"

namespace arkostracker
{

class MainController;

class MergeSong final : public LoadSong::Callback
{
public:
    explicit MergeSong(MainController& mainController) noexcept;

    /** Opens a file picker, merges the song. */
    void merge();

    // LoadSong::Callback implementation
    // ==========================================
    void onLoadedSongFailure() override;
    void onLoadedSongSuccess(std::unique_ptr<Song> song) override;

private:
    /** Merges the given song into the current one. */
    void merge(const Song& songToMerge) noexcept;

    MainController& mainController;
    std::unique_ptr<ModalDialog> modalDialog;
    std::unique_ptr<LoadSong> loadSong;
};

}   // namespace arkostracker
