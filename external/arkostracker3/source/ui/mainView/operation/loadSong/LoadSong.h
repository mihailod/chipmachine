#pragma once

#include "../../../../import/loader/SongLoader.h"
#include "../../../../song/Song.h"
#include "../../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../../import/ImportPanel.h"
#include "../../../utils/backgroundOperation/BackgroundOperationWithDialog.h"
#include "../CheckIfSongSavedAndSave.h"

namespace arkostracker 
{

class MainController;
class ImportFirstPassReturnData;

/**
 * Asynchronously loads of the song, opening a file picker (if no path is given), checking if the current song is modified,
 * asking to open the configuration panels if needed, etc.
 *
 * All the dialogs (error, etc.) are managed here.
 *
 * In case of success, the Preferences is added the path of the Song. If case the file doesn't exist, it is removed.
 */
class LoadSong
{
public:
    /** Callback for clients to be notified of the result. */
    class Callback
    {
    public:
        /** Destructor. */
        virtual ~Callback() = default;

        /**
         * Called when the Song has been successfully loaded. The caller does not have to show any dialog, it was already handled here.
         * @param song the Song.
         */
        virtual void onLoadedSongSuccess(std::unique_ptr<Song> song) = 0;
        /** Called when the loading of the Song has failed. */
        virtual void onLoadedSongFailure() = 0;
    };

    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param callback the callback to be notified of the result.
     * @param setSongPath true to store the loaded song path as the path to save to. When merging, set to false.
     */
    explicit LoadSong(MainController& mainController, Callback& callback, bool setSongPath) noexcept;

    /**
     * Starts the process of loading a song.
     * @param possiblePathToOpen if not empty, will use it instead of opening a file picker. The path must be absolute.
     */
    void startLoadSongProcess(const juce::String& possiblePathToOpen) noexcept;

private:
    /**
     * Called when the operation finishes.
     * @param result the result, if successful.
     */
    void onBackgroundOperationFinished(std::unique_ptr<SongLoader::Result> result) noexcept;

    /**
     * Loads the song. The save check has been done.
     * @param possiblePathToOpen if not empty, will use it instead of opening a file picker. The path must be absolute.
     */
    void loadSongAfterSaveCheck(const juce::String& possiblePathToOpen) noexcept;

    /**
     * Shows a generic error dialog, and exits when clicked.
     * @param fileMissing true if the file is absent, false for all the other reasons.
     */
    void showErrorDialogAndExit(bool fileMissing) noexcept;

    /** Called after the Error Report has been shown and removed. */
    void afterErrorReportShown() noexcept;

    /** Calls the success callback. Also marks the song as "in recent files". The internal Song must be set. */
    void exitForSuccess() noexcept;
    /** Shows a "file not found" dialog, removes the song from the "recent files", and calls the failure callback. */
    void exitBecauseFileNotFound() noexcept;

    /**
     * Called when a configuration is needed. Shows the panel, then retries.
     * @param configurationType the missing configuration type.
     * @param importFirstPassReturnData the data returned by the importer to inject into the import panel.
     */
    void manageConfigurationNeeded(ConfigurationType configurationType, const ImportFirstPassReturnData& importFirstPassReturnData) noexcept;

    /** Called when OK is clicked on the import configuration panel. */
    void onImportConfigurationPanelOk() noexcept;
    /** Called when the import configuration panel is cancelled. */
    void onImportConfigurationPanelCancel() noexcept;

    /** Starts the import asynchronously, from the known internal file. */
    void startImportWithKnownFile() noexcept;

    MainController& mainController;
    Callback& callback;
    bool setSongPath;
    std::unique_ptr<juce::DialogWindow> dialog;
    std::unique_ptr<BackgroundOperationWithDialog<std::unique_ptr<SongLoader::Result>>> backgroundOperation;
    juce::File fileToLoad;
    bool isNativeFormat;

    std::unique_ptr<ImportConfiguration> importConfiguration;       // Null at first, set when the user sets it.
    std::unique_ptr<ImportPanel> importPanel;

    std::unique_ptr<ErrorReport> errorReport;
    std::unique_ptr<Song> song;
    std::unique_ptr<CheckIfSongSavedAndSave> checkIfSongSavedAndSave;
};

}   // namespace arkostracker
