#pragma once

#include "../components/dialogs/ModalDialog.h"
#include "../../utils/OptionalValue.h"
#include "../components/EditText.h"

namespace arkostracker 
{

class MainController;

/** Shows the Song Properties dialog. This class has no cleverness at all. */
class SongPropertiesDialog : public ModalDialog
{
public:
    /** The result, holding all that has changed (if any). */
    class Result
    {
    public:
        Result(const OptionalValue<juce::String>& pNewTitle, const OptionalValue<juce::String>& pNewAuthor, const OptionalValue<juce::String>& pNewComposer,
               const OptionalValue<juce::String>& pNewComments) :
               newTitle(pNewTitle),
               newAuthor(pNewAuthor),
               newComposer(pNewComposer),
               newComments(pNewComments)
        {
        }

        const OptionalValue<juce::String> newTitle;
        const OptionalValue<juce::String> newAuthor;
        const OptionalValue<juce::String> newComposer;
        const OptionalValue<juce::String> newComments;
    };

    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param okCallback called when the OK button is clicked, with the data that has changed (if any).
     * @param cancelCallback called when the Cancel button is clicked.
     */
    SongPropertiesDialog(const MainController & mainController, std::function<void(SongPropertiesDialog::Result)> okCallback, const std::function<void()>& cancelCallback) noexcept;

private:
    /** Called when OK is clicked. */
    void onOkClicked() const noexcept;

    std::function<void(SongPropertiesDialog::Result)> okCallback;

    juce::String originalTitle;
    juce::String originalAuthor;
    juce::String originalComposer;
    juce::String originalComments;

    juce::Label titleLabel;
    EditText titleTextEditor;
    juce::Label authorLabel;
    EditText authorTextEditor;
    juce::Label composerLabel;
    EditText composerTextEditor;
    juce::Label commentsLabel;
    EditText commentsTextEditor;
};

}   // namespace arkostracker
