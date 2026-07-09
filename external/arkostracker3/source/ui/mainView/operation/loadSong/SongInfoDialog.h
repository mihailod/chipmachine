#pragma once

#include "../../../components/dialogs/ModalDialog.h"
#include "../../../components/EditText.h"

namespace arkostracker 
{

/** Shows the info (read-only) about the loaded Song. It takes care by itself of managing the Preferences. */
class SongInfoDialog : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param title the song title.
     * @param author the author.
     * @param composer the composer.
     * @param comments the comments.
     * @param okCallback the callback when OK/cancel is pressed.
     */
    SongInfoDialog(const juce::String& title, const juce::String& author, const juce::String& composer, const juce::String& comments, std::function<void()> okCallback) noexcept;

private:
    /** Called when OK/cancel buttons are clicked. */
    void validate(bool okPressed) const noexcept;

    std::function<void()> okCallback;

    juce::Label titleLabel;
    EditText titleTextEditor;
    juce::Label authorLabel;
    EditText authorTextEditor;
    juce::Label composerLabel;
    EditText composerTextEditor;
    juce::Label commentsLabel;
    EditText commentsTextEditor;

    juce::ToggleButton showOnLoadToggle;
};

}   // namespace arkostracker
