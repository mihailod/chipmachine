#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ButtonWithImage.h"
#include "EditText.h"

namespace arkostracker
{

/**
 * Component with a path and an button to load a file.
 * Can also manage drag'n'drop.
 */
class FilePathChooser final : public juce::Component,
                              public juce::FileDragAndDropTarget                   // To detect drag'n'drop of external files.

{
public:
    static const int desiredHeight;

    /**
     * Constructor.
     * @param labelText the text for the label ("Path to WAV" for example).
     * @param fileChooserText the text for the file chooser ("Browse for WAV").
     * @param fileExtensionWithWildcard the file extension ("*.wav") to look for.
     */
    FilePathChooser(const juce::String& labelText, juce::String fileChooserText, juce::String fileExtensionWithWildcard) noexcept;

    // Component methods implementation.
    // ===================================================================
    void resized() override;

    // FileDragAndDropTarget methods implementation.
    // ===================================================================
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // ===================================================================

    /** @return the selected path. May be invalid. Check isSelectedFileValid first as a convenience. */
    juce::File getSelectedFile() const noexcept;

    /** @return true if the selected file exists as a file. */
    bool isSelectedFileValid() const noexcept;

private:
    /** Opens the browser to select a path. */
    void onPathButtonClicked() noexcept;

    juce::Label label;
    EditText pathEditText;
    ButtonWithImage pathButton;

    juce::String fileChooserText;
    juce::String fileExtensionWithWildcard;
};

}   // namespace arkostracker
