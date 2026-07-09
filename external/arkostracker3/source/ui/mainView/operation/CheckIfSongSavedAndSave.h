#pragma once

#include <functional>
#include <memory>

#include "../../components/dialogs/ModalDialog.h"
#include "SaveSong.h"

namespace arkostracker 
{

class MainController;

/** Checks whether the Song is saved. If not, asks the user what he wants to do. If wanting to save, saves the song. */
class CheckIfSongSavedAndSave final
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param callback called with "true" if everything went fine (song saved or not), false if the operation was in error or canceled.
     */
    explicit CheckIfSongSavedAndSave(MainController& mainController, std::function<void(bool)> callback) noexcept;

    /** Performs the check and the operations. */
    void perform() noexcept;

private:
    /** Called when the user wants to save the song. */
    void onSaveClicked() noexcept;

    /** Hides the Song Not Saved Dialog. It does not delete the object! */
    void hideSongNotSavedDialog() const noexcept;

    MainController& mainController;
    std::function<void(bool)> callback;

    std::unique_ptr<ModalDialog> dialog;
    std::unique_ptr<SaveSong> saveSong;
};

}   // namespace arkostracker
