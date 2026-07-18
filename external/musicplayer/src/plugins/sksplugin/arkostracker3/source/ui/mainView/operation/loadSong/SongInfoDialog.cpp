#include "SongInfoDialog.h"

#include "../../../../app/preferences/PreferencesManager.h"

namespace arkostracker 
{

SongInfoDialog::SongInfoDialog(const juce::String& pTitle, const juce::String& pAuthor, const juce::String& pComposer,
                               const juce::String& pComments, std::function<void()> pOkCallback) noexcept :
        ModalDialog(juce::translate("Song information"), 600, 340,
                    [&]{ validate(true); }, [&]{ validate(false); }),
        okCallback(std::move(pOkCallback)),

        titleLabel(juce::String(), juce::translate("Song title")),
        titleTextEditor(pTitle, false),
        authorLabel(juce::String(), juce::translate("Author")),
        authorTextEditor(pAuthor, false),
        composerLabel(juce::String(), juce::translate("Composer")),
        composerTextEditor(pComposer, false),
        commentsLabel(juce::String(), juce::translate("Comments")),
        commentsTextEditor(pComments, false, true),

        showOnLoadToggle(juce::translate("Show this when loading a song."))
{
    const auto bounds = getUsableModalDialogBounds();
    constexpr auto labelsWidth = 90;
    constexpr auto labelsHeight = 30;
    constexpr auto yMargin = 20;
    constexpr auto xMargin = 10;

    const auto x = bounds.getX();
    auto y = bounds.getY();
    const auto width = bounds.getWidth();
    const auto middleX = (bounds.getWidth() / 2) + x;
    const auto xEditor = x + labelsWidth;
    const auto editorWidth = width - xEditor;

    titleLabel.setBounds(x, y, labelsWidth, labelsHeight);
    titleTextEditor.setBounds(xEditor, y, editorWidth, labelsHeight);
    y += labelsHeight + yMargin;

    authorLabel.setBounds(x, y, labelsWidth, labelsHeight);
    authorTextEditor.setBounds(xEditor, y, middleX - xEditor - xMargin, labelsHeight);
    composerLabel.setBounds(authorTextEditor.getRight() + xMargin * 3, y, labelsWidth, labelsHeight);
    const auto composerTextEditorX = composerLabel.getRight();
    composerTextEditor.setBounds(composerTextEditorX, y, width - composerTextEditorX, labelsHeight);

    y += labelsHeight + yMargin;
    commentsLabel.setBounds(x, y, labelsWidth, labelsHeight);
    commentsTextEditor.setBounds(xEditor, y, editorWidth, labelsHeight * 4);

    y = commentsTextEditor.getBottom() + yMargin / 2;
    showOnLoadToggle.setBounds(x, y, width, labelsHeight);

    addComponentToModalDialog(titleLabel);
    addComponentToModalDialog(titleTextEditor);
    addComponentToModalDialog(authorLabel);
    addComponentToModalDialog(authorTextEditor);
    addComponentToModalDialog(composerLabel);
    addComponentToModalDialog(composerTextEditor);
    addComponentToModalDialog(commentsLabel);
    addComponentToModalDialog(commentsTextEditor);
    addComponentToModalDialog(showOnLoadToggle);

    // Sets the data.
    titleTextEditor.setText(pTitle);
    authorTextEditor.setText(pAuthor);
    composerTextEditor.setText(pComposer);
    commentsTextEditor.setText(pComments);

    const auto showOnLoad = PreferencesManager::getInstance().isSongInfoShownAfterLoad();
    showOnLoadToggle.setToggleState(showOnLoad, juce::NotificationType::dontSendNotification);
}

void SongInfoDialog::validate(const bool okPressed) const noexcept
{
    // Stores the "show" new state.
    if (okPressed) {
        const auto showOnLoad = showOnLoadToggle.getToggleState();
        PreferencesManager::getInstance().setShowSongInfoAfterLoad(showOnLoad);
    }

    okCallback();
}

}   // namespace arkostracker
