#include "CheckIfSongSavedAndSave.h"

#include "../../../controllers/MainController.h"
#include "../../components/dialogs/SongNotSavedDialog.h"

namespace arkostracker 
{

CheckIfSongSavedAndSave::CheckIfSongSavedAndSave(MainController& pMainController, std::function<void(bool)> pCallback) noexcept :
        mainController(pMainController),
        callback(std::move(pCallback)),
        dialog(),
        saveSong()
{
}

void CheckIfSongSavedAndSave::perform() noexcept
{
    // Is the song modified? If not, there is nothing to do.
    if (!mainController.isSongModified()) {
        callback(true);
        return;
    }

    // The song is modified. Does the user want to save it?
    jassert(dialog == nullptr);        // Shouldn't exist yet.
    dialog = std::make_unique<SongNotSavedDialog>([&] {
                                                      onSaveClicked();          // Saves the song.
                                                  },
                                                  [&] {
                                                      hideSongNotSavedDialog();
                                                      callback(true);           // Don't save.
                                                  },
                                                  [&] {
                                                      hideSongNotSavedDialog();
                                                      callback(false);          // Cancel.
                                                  });
}

void CheckIfSongSavedAndSave::onSaveClicked() noexcept
{
    // Saves the Song.
    jassert(saveSong == nullptr);           // Already present?
    saveSong = std::make_unique<SaveSong>(false, mainController, [&](const bool success) {
        callback(success);
    });
    saveSong->saveSong();
}

void CheckIfSongSavedAndSave::hideSongNotSavedDialog() const noexcept
{
    dialog->hide();
}

}   // namespace arkostracker
