#pragma once

#include <functional>

#include "../../../song/Song.h"
#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

class MainController;

/**
 * Performs the saving of a Song asynchronously, showing the file picker if no path is already known.
 * Also manages an error dialog in case the process failed, no need for the client to do it.
 */
class SaveSong
{
public:
    /**
     * Constructor.
     * @param forceAskForPath true to force the file picker. Useful for "save as".
     * @param mainController the Main Controller.
     * @param callback the callback to be notified of the result, true if successful. False if error, or user canceled.
     */
    SaveSong(bool forceAskForPath, MainController& mainController, std::function<void(bool)> callback) noexcept;

    /** Saves the Song asynchronously. Note: the song WILL be modified to update the modification date. */
    void saveSong() noexcept;

private:
    /**
     * Called when the operation finishes.
     * @param success true if success.
     */
    void onBackgroundOperationFinished(bool success) noexcept;

    const bool forceAskForPath;
    MainController& mainController;
    std::shared_ptr<Song> song;
    const bool compressXml;
    std::function<void(bool)> callback;

    std::unique_ptr<juce::DialogWindow> dialog;
    std::unique_ptr<BackgroundOperationWithDialog<bool>> backgroundOperation;

    juce::File fileToSaveTo;
};

}   // namespace arkostracker
