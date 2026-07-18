#include "FileChooserCustom.h"

#include "../../app/preferences/LastFolderTempStorage.h"
#include "../../utils/StringUtil.h"

namespace arkostracker 
{

juce::File FileChooserCustom::save(const Target target, const juce::String& extensionWithoutDot, const bool deleteTargetFile) noexcept
{
    const auto extensionFilter = extensionWithoutDot.isEmpty() ? "*" : ("*." + extensionWithoutDot);

    juce::String title;
    switch (target) {
        case ym:
            title = juce::translate("Save as YM");
            break;
        case akg:
            title = juce::translate("Save as AKG");
            break;
        case akm:
            title = juce::translate("Save as AKM");
            break;
        case aky:
            title = juce::translate("Save as AKY");
            break;
        case fap:
            title = juce::translate("Save as FAP");
            break;
        case midi:
            title = juce::translate("Save as MIDI");
            break;
        case mod:
            title = juce::translate("Save as MOD");
            break;
        case events:
            title = juce::translate("Save events");
            break;
        case samples:
            title = juce::translate("Save samples");
            break;
        case raw:
            title = juce::translate("Save as RAW");
            break;
        case txt:
            title = juce::translate("Save as TXT");
            break;
        case akx:
            title = juce::translate("Save as AKX");
            break;
        case vgm:
            title = juce::translate("Save as VGM");
            break;
        case wav:
            title = juce::translate("Save as WAV");
            break;
        default:
            jassertfalse;       // Abnormal.
        case any:
            title = juce::translate("Save");
            break;
    }
    // The context is "any" export, there is not a specific folder per export.
    FileChooserCustom fileChooser(title, FolderContext::anyExport, extensionFilter);
    const auto success = fileChooser.browseForFileToSave(true);
    if (!success) {
        return { };                    // Cancellation from the user.
    }

    auto fileToSave = fileChooser.getResultWithExtensionIfNone(extensionFilter);
    if (deleteTargetFile) {
        (void)fileToSave.deleteFile();
    }

    return fileToSave;
}

FileChooserCustom::FileChooserCustom(const juce::String& pDialogBoxTitle, const FolderContext pFolderContext, const juce::String& pFilePatternsAllowed,
                                     const juce::String& pFileNameToLoad, const bool useOsNativeDialogBox, const bool pTreatFilePackagesAsDirectories) :
    folderContext(pFolderContext),
    fileChooser(pDialogBoxTitle, getInitialFolder(pFileNameToLoad), pFilePatternsAllowed, useOsNativeDialogBox, pTreatFilePackagesAsDirectories)
{
}

juce::File FileChooserCustom::getInitialFolder(const juce::String& fileNameToLoad) const noexcept
{
    // Gets the last folder known for this context.
    const auto folderString = LastFolderTempStorage::getRecentFolder(folderContext);
    // To return an absolute path, else JUCE assertion raises if the path is empty, on Windows.
    auto folder = juce::File::getCurrentWorkingDirectory().getChildFile(folderString);
    // Is it valid? It must not be a file.
    if (!folder.exists() || folder.existsAsFile()) {
        folder = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory);       // Fallback folder is error.
    }
    // Appends the file to the folder.
    return folder.getChildFile(juce::File::createLegalFileName(fileNameToLoad));  // Removes illegal chars, the file name could be invalid!
}

bool FileChooserCustom::browseForFileToOpen(std::unique_ptr<juce::FilePreviewComponent> paramPreviewComponent)
{
    // Stores it!
    previewComponent = std::move(paramPreviewComponent);

    const auto success = fileChooser.browseForFileToOpen(previewComponent.get());

    saveFolderIfSuccess(success, fileChooser.getResult(), false);

    return success;
}

bool FileChooserCustom::browseForMultipleFilesToOpen(juce::FilePreviewComponent* paramPreviewComponent)
{
    const auto success = fileChooser.browseForMultipleFilesToOpen(paramPreviewComponent);

    // Stores the first folder.
    const auto files = fileChooser.getResults();
    if (files.size() > 0) {
        saveFolderIfSuccess(success, files[0], false);
    }

    return success;
}

bool FileChooserCustom::browseForFileToSave(const bool warnAboutOverwritingExistingFiles)
{
    const auto success = fileChooser.browseForFileToSave(warnAboutOverwritingExistingFiles);

    saveFolderIfSuccess(success, fileChooser.getResult(), false);

    return success;
}

bool FileChooserCustom::browseForDirectory()
{
    const auto success = fileChooser.browseForDirectory();

    saveFolderIfSuccess(success, fileChooser.getResult(), true);

    return success;
}

bool FileChooserCustom::browseForMultipleFilesOrDirectories(juce::FilePreviewComponent* paramPreviewComponent)
{
    const auto success = fileChooser.browseForMultipleFilesOrDirectories(paramPreviewComponent);

    // Stores the first folder.
    const auto files = fileChooser.getResults();
    if (files.size() > 0) {
        saveFolderIfSuccess(success, files[0], false);      // Impossible to know if true/false...
    }

    return success;
}

void FileChooserCustom::saveFolderIfSuccess(const bool success, const juce::File& result, const bool isFolder) const noexcept
{
    // The user cancelled? Don't store the folder, then.
    if (!success) {
        return;
    }

    // Don't take the file itself, take the parent folder, if the result is a file.
    const auto folderToSave = isFolder ? result : result.getParentDirectory();
    LastFolderTempStorage::setRecentFolder(folderContext, folderToSave);
}

juce::File FileChooserCustom::getResult() const
{
    return fileChooser.getResult();
}

juce::File FileChooserCustom::getResultWithExtensionIfNone(const juce::String& extensionMaybeWithDotOrFilter) const
{
    auto file = fileChooser.getResult();
    // Is there any extension? If not, add a new one.
    const auto fileName = file.getFileName();
    // Skips the first char because hidden files starts with ".", don't consider them as an extension.
    if (!fileName.substring(1).containsChar('.')) {
        // There is no extension.
        const auto extensionWithDot = StringUtil::extractExtension(extensionMaybeWithDotOrFilter, true);
        return { file.getFullPathName() + extensionWithDot };
    }
    return file;
}

}   // namespace arkostracker
