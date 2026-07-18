#pragma once

#include <juce_core/juce_core.h>

#include "FolderContext.h"

namespace arkostracker
{

/**
 * Convenient static class which methods keep the latest folders, so that classes can get/set it without having to manipulate the Preferences.
 * However, the folders must set/store it at the beginning of the application, and on exit!
 */
class LastFolderTempStorage
{
public:
    /**
     * Sets recent folder, to go there on next time.
     * This must be called only from the UI thread.
     * @param folderContext the context for the folder to use.
     * @param folderOrFile a folder, but may be file, in which case its folder will be extracted. May also be an invalid folder, in which case the recent folder is reset.
     */
    static void setRecentFolder(FolderContext folderContext, const juce::File& folderOrFile) noexcept;

    /**
     * Returns the most recent folder where the user has been. If there is none, an empty String is returned.
     * This must be called only from the UI thread.
     * @param folderContext the context for the folder to use.
     */
    static juce::String getRecentFolder(FolderContext folderContext) noexcept;

private:
    /**
     * Returns a reference to the folder path for the given context.
     * @param folderContext the context.
     * @return the folder reference. May be empty.
     */
    static juce::String& getFolder(FolderContext folderContext) noexcept;

    static juce::String folderForLoadSaveSong;          // The last known folder for loading/saving a Song.
    static juce::String folderForLoadSaveInstrument;    // The last known folder for loading/saving an Instrument.
    static juce::String folderForLoadSaveArpeggio;      // The last known folder for loading/saving an Arpeggio.
    static juce::String folderForLoadSavePitch;         // The last known folder for loading/saving a Pitch.
    static juce::String folderForAnyExport;             // The last known folder for any export.
    static juce::String folderForOther;                 // The last known folder for any other use.
};

}   // namespace arkostracker
