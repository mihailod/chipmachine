#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../app/preferences/FolderContext.h"

namespace arkostracker 
{

/**
 * Class that is only a wrapper around the JUCE FileChooser, that transmits events to it, but when the result is a success, saves the
 * selected folder to the static LastFolderTempStorage.
 * Also, the initial folder, if empty (invalid file) or if using the right constructor, is selected automatically from the same static storage.
 */
class FileChooserCustom
{
public:
    /** What to load/save. */
    enum Target: uint8_t
    {
        akg,
        akm,
        aky,
        akx,
        fap,
        midi,
        mod,
        ym,
        vgm,
        wav,
        events,
        samples,
        raw,
        txt,

        any,
    };

    /**
     * Convenience function to open a FileChooser for saving. The file is automatically deleted if validated.
     * @param target what is saved.
     * @param extensionWithoutDot the extension, without the dot. If empty, uses "*" finally.
     * @param deleteTargetFile true to delete the target file.
     * @return the file, if success, or empty file if cancelled (use getFullPathName().isEmpty()).
     */
    static juce::File save(Target target, const juce::String& extensionWithoutDot, bool deleteTargetFile = true) noexcept;


    //==============================================================================
    /** Creates a FileChooser.

    After creating one of these, use one of the browseFor... methods to display it.

    @param dialogBoxTitle                 a text string to display in the dialog box to
    tell the user what's going on
    @param folderContext                  a context to know what folder to choose.
    @param filePatternsAllowed            a set of file patterns to specify which files
    can be selected - each pattern should be
    separated by a comma or semi-colon, e.g. "*" or
    "*.jpg;*.gif". An empty string means that all
    files are allowed
    @param fileNameToLoad                 if not empty, adds this to the folder to allow
    a file to be first set. Note that illegal chars are removed from the given name.
    @param useOsNativeDialogBox           if true, then a native dialog box will be used
    if possible; if false, then a Juce-based
    browser dialog box will always be used
    @param treatFilePackagesAsDirectories if true, then the file chooser will allow the
    selection of files inside packages when
    invoked on OS X and when using native dialog
    boxes.

    @see browseForFileToOpen, browseForFileToSave, browseForDirectory
    */
    explicit FileChooserCustom(const juce::String& dialogBoxTitle,
        FolderContext folderContext,
        const juce::String& filePatternsAllowed = juce::String(),
        const juce::String& fileNameToLoad = juce::String(),
        bool useOsNativeDialogBox = true,
        bool treatFilePackagesAsDirectories = false);

    //==============================================================================
    /** Shows a dialog box to choose a file to open.

    This will display the dialog box modally, using an "open file" mode, so that
    it won't allow non-existent files or directories to be chosen.

    @param previewComponent   an optional component to display inside the dialog
    box to show special info about the files that the user
    is browsing.
    @returns    true if the user selected a file, in which case, use the getResult()
    method to find out what it was. Returns false if they cancelled instead.
    @see browseForFileToSave, browseForDirectory
    */
    bool browseForFileToOpen(std::unique_ptr<juce::FilePreviewComponent> previewComponent = nullptr);

    /** Same as browseForFileToOpen, but allows the user to select multiple files.

    The files that are returned can be obtained by calling getResults(). See
    browseForFileToOpen() for more info about the behaviour of this method.
    */
    bool browseForMultipleFilesToOpen(juce::FilePreviewComponent* previewComponent = nullptr);

    /** Shows a dialog box to choose a file to save.

    This will display the dialog box modally, using an "save file" mode, so it
    will allow non-existent files to be chosen, but not directories.

    @param warnAboutOverwritingExistingFiles     if true, the dialog box will ask
    the user if they're sure they want to overwrite a file that already
    exists
    @returns    true if the user chose a file and pressed 'ok', in which case, use
    the getResult() method to find out what the file was. Returns false
    if they cancelled instead.
    @see browseForFileToOpen, browseForDirectory
    */
    bool browseForFileToSave(bool warnAboutOverwritingExistingFiles);

    /** Shows a dialog box to choose a directory.

    This will display the dialog box modally, using an "open directory" mode, so it
    will only allow directories to be returned, not files.

    @returns    true if the user chose a directory and pressed 'ok', in which case, use
    the getResult() method to find out what they chose. Returns false
    if they cancelled instead.
    @see browseForFileToOpen, browseForFileToSave
    */
    bool browseForDirectory();

    /** Same as browseForFileToOpen, but allows the user to select multiple files and directories.

    The files that are returned can be obtained by calling getResults(). See
    browseForFileToOpen() for more info about the behaviour of this method.
    */
    bool browseForMultipleFilesOrDirectories(juce::FilePreviewComponent* previewComponent = nullptr);

    //==============================================================================
    /** Runs a dialog box for the given set of option flags.
    The flag values used are those in FileBrowserComponent::FileChooserFlags.

    @returns    true if the user chose a directory and pressed 'ok', in which case, use
    the getResult() method to find out what they chose. Returns false
    if they cancelled instead.
    @see FileBrowserComponent::FileChooserFlags
    */
    //bool showDialog(int flags, FilePreviewComponent* previewComponent);

    //==============================================================================
    /** Returns the last file that was chosen by one of the browseFor methods.

    After calling the appropriate browseFor... method, this method lets you
    find out what file or directory they chose.

    Note that the file returned is only valid if the browse method returned true (i.e.
    if the user pressed 'ok' rather than cancelling).

    If you're using a multiple-file select, then use the getResults() method instead,
    to obtain the list of all files chosen.

    NOTE AT3: This method, on Linux, may NOT return the file with the extension, when SAVING. So this is the original JUCE method.
    Use getResultWithExtension to have the extension append if not present yet.

    @see getResults
    */
    juce::File getResult() const;

    /**
     * @see getResult. This method appends the given extension IF NO extension is present yet in the returned File.
     * This should not be used for loading a file, else an extension will be added!
     * @param extensionMaybeWithDotOrFilter can be "aks", ".aks", "*.aks" or even "stuff.aks" (searches for the first dot to know where to start).
     */
    juce::File getResultWithExtensionIfNone(const juce::String& extensionMaybeWithDotOrFilter) const;

    /** Returns a list of all the files that were chosen during the last call to a
    browse method.

    This array may be empty if no files were chosen, or can contain multiple entries
    if multiple files were chosen.

    @see getResult
    */
    juce::Array<juce::File> getResults() const noexcept { return fileChooser.getResults(); }

private:
    /**
     * Stores the folder from the FileChooser, but only if the latter operation is a success (else, it means the user cancelled).
     * @param success the success state of the FileChooser. If false, nothing happens.
     * @param result the result of the FileChooser.
     * @param isFolder true if the result is a folder, false if a file. Needed to know if the parent must be stored (false) or directly the result (true).
     */
    void saveFolderIfSuccess(bool success, const juce::File& result, bool isFolder) const noexcept;

    /**
     * Returns the folder related to the current folder context.
     * @param fileNameToLoad the name of the file to load.
     * @return the folder.
     */
    juce::File getInitialFolder(const juce::String& fileNameToLoad) const noexcept;

    FolderContext folderContext;
    juce::FileChooser fileChooser;

    std::unique_ptr<juce::FilePreviewComponent> previewComponent;         // The possible PreviewComponent.
};

}   // namespace arkostracker
