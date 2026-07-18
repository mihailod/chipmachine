#include "FilePathChooser.h"

#include <BinaryData.h>

#include "../../utils/FileUtil.h"
#include "../lookAndFeel/LookAndFeelConstants.h"
#include "FileChooserCustom.h"

namespace arkostracker
{

const int FilePathChooser::desiredHeight = 25 * 2;

FilePathChooser::FilePathChooser(const juce::String& pLabelText, juce::String pFileChooserText, juce::String pFileExtensionWithWildcard) noexcept :
        label(juce::String(), pLabelText),
        pathEditText(),
        pathButton(
          BinaryData::IconFolder_png, BinaryData::IconFolder_pngSize, juce::translate("Browse..."),
          [&] (int, bool, bool) { onPathButtonClicked(); }),
        fileChooserText(std::move(pFileChooserText)),
        fileExtensionWithWildcard(std::move(pFileExtensionWithWildcard))
{
    addAndMakeVisible(label);
    addAndMakeVisible(pathEditText);
    addAndMakeVisible(pathButton);
}


// Component methods implementation.
// ===================================================================

void FilePathChooser::resized()
{
    constexpr auto x = 0;
    constexpr auto top = 0;
    constexpr auto sampleInstrumentPathButtonWidth = 32;

    const auto width = getWidth();
    const auto smallMargins = LookAndFeelConstants::margins / 2;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    label.setBounds(x, top, width, labelsHeight);
    pathButton.setBounds(width - sampleInstrumentPathButtonWidth + x, label.getBottom(), sampleInstrumentPathButtonWidth, labelsHeight);
    pathEditText.setBounds(x, pathButton.getY(), pathButton.getX() - x - smallMargins, labelsHeight);
}


// FileDragAndDropTarget methods implementation.
// ===================================================================

bool FilePathChooser::isInterestedInFileDrag(const juce::StringArray& /*files*/)
{
    return true;
}

void FilePathChooser::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.isEmpty()) {
        return;
    }

    // Takes the first file, checks its validity.
    const auto& filePath = files[0];
    if (const auto file = FileUtil::getFileFromString(filePath); file.existsAsFile()) {
        pathEditText.setText(filePath);
    }
}


// ===================================================================

void FilePathChooser::onPathButtonClicked() noexcept
{
    FileChooserCustom fileChooser(fileChooserText, FolderContext::other, fileExtensionWithWildcard);
    if (const auto success = fileChooser.browseForFileToOpen(nullptr); !success) {
        return;
    }

    const auto fileToLoad = fileChooser.getResultWithExtensionIfNone(fileExtensionWithWildcard);
    pathEditText.setText(fileToLoad.getFullPathName(), false);
}

juce::File FilePathChooser::getSelectedFile() const noexcept
{
    return FileUtil::getFileFromString(pathEditText.getText());
}

bool FilePathChooser::isSelectedFileValid() const noexcept
{
    return getSelectedFile().existsAsFile();
}

}   // namespace arkostracker
