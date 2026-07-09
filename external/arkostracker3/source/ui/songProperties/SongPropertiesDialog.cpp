#include "SongPropertiesDialog.h"

#include "../../controllers/MainController.h"

namespace arkostracker 
{

SongPropertiesDialog::SongPropertiesDialog(const MainController & pMainController, std::function<void(SongPropertiesDialog::Result)> pOkCallback,
                                           const std::function<void()>& pCancelCallback) noexcept :
        ModalDialog(juce::translate("Song properties"), 600, 300, [&] { onOkClicked(); }, pCancelCallback,
                    true, true),
        okCallback(std::move(pOkCallback)),

        originalTitle(),
        originalAuthor(),
        originalComposer(),
        originalComments(),

        titleLabel(juce::String(), juce::translate("Song title")),
        titleTextEditor(juce::String()),
        authorLabel(juce::String(), juce::translate("Author")),
        authorTextEditor(juce::String()),
        composerLabel(juce::String(), juce::translate("Composer")),
        composerTextEditor(juce::String()),
        commentsLabel(juce::String(), juce::translate("Comments")),
        commentsTextEditor(juce::String())
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
    composerLabel.setBounds(authorTextEditor.getRight() + (xMargin * 3), y, labelsWidth, labelsHeight);
    const auto composerTextEditorX = composerLabel.getRight();
    composerTextEditor.setBounds(composerTextEditorX, y, width - composerTextEditorX, labelsHeight);

    y += labelsHeight + yMargin;
    commentsLabel.setBounds(x, y, labelsWidth, labelsHeight);
    commentsTextEditor.setBounds(xEditor, y, editorWidth, labelsHeight * 4);
    commentsTextEditor.setMultiLine(true, true);
    commentsTextEditor.setReturnKeyStartsNewLine(true);

    addComponentToModalDialog(titleLabel);
    addComponentToModalDialog(titleTextEditor);
    addComponentToModalDialog(authorLabel);
    addComponentToModalDialog(authorTextEditor);
    addComponentToModalDialog(composerLabel);
    addComponentToModalDialog(composerTextEditor);
    addComponentToModalDialog(commentsLabel);
    addComponentToModalDialog(commentsTextEditor);

    // Fills the fields.
    const auto& songController = pMainController.getSongController();
    originalTitle = songController.getTitle();
    originalAuthor = songController.getAuthor();
    originalComposer = songController.getComposer();
    originalComments = songController.getComments();
    titleTextEditor.setText(originalTitle, false);
    authorTextEditor.setText(originalAuthor, false);
    composerTextEditor.setText(originalComposer, false);
    commentsTextEditor.setText(originalComments, false);

    giveFocus(titleTextEditor);
}

void SongPropertiesDialog::onOkClicked() const noexcept
{
    const auto newTitle = titleTextEditor.getText();
    const auto newAuthor = authorTextEditor.getText();
    const auto newComposer = composerTextEditor.getText();
    const auto newComments = commentsTextEditor.getText();

    // Returns the info that have changed.
    const Result result(
            (newTitle != originalTitle) ? newTitle : OptionalValue<juce::String>(),
            (newAuthor != originalAuthor) ? newAuthor : OptionalValue<juce::String>(),
            (newComposer != originalComposer) ? newComposer : OptionalValue<juce::String>(),
            (newComments != originalComments) ? newComments : OptionalValue<juce::String>());

    okCallback(result);
}

}   // namespace arkostracker
