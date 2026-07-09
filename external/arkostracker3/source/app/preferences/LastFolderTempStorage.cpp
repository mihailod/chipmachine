#include "LastFolderTempStorage.h"

namespace arkostracker
{

juce::String LastFolderTempStorage::folderForLoadSaveSong = juce::String();             // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
juce::String LastFolderTempStorage::folderForLoadSaveInstrument = juce::String();       // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
juce::String LastFolderTempStorage::folderForLoadSaveArpeggio = juce::String();         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
juce::String LastFolderTempStorage::folderForLoadSavePitch = juce::String();            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
juce::String LastFolderTempStorage::folderForAnyExport = juce::String();                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
juce::String LastFolderTempStorage::folderForOther = juce::String();                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

juce::String LastFolderTempStorage::getRecentFolder(const FolderContext folderContext) noexcept
{
    return getFolder(folderContext);
}

void LastFolderTempStorage::setRecentFolder(const FolderContext folderContext, const juce::File& folderOrFile) noexcept
{
    juce::String pathToSave;
    if (folderOrFile.existsAsFile()) {
        // It is a file. Extracts the path.
        pathToSave = folderOrFile.getParentDirectory().getFullPathName();
    } else {
        pathToSave = folderOrFile.getFullPathName();
    }

    // Replaces the value of the folder.
    auto& folder = getFolder(folderContext);
    folder = pathToSave;
}

juce::String& LastFolderTempStorage::getFolder(const FolderContext folderContext) noexcept
{
    switch (folderContext) {
        default:
            jassertfalse;   // Missing folder. Fall-through.
        case FolderContext::anyExport: return folderForAnyExport;
        case FolderContext::loadOrSaveSong: return folderForLoadSaveSong;
        case FolderContext::loadOrSaveInstrument: return folderForLoadSaveInstrument;
        case FolderContext::loadOrSaveArpeggio: return folderForLoadSaveArpeggio;
        case FolderContext::loadOrSavePitch: return folderForLoadSavePitch;
        case FolderContext::other: return folderForOther;
    }
}

}   // namespace arkostracker
