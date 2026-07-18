#pragma once

#include "../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

/** Dialog to create a new song. */
class NewSongDialog final : public ModalDialog
{
public:
    /** To be aware of the events of the Dialog. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called if the user validated the new song parameters.
         * @param selectedTemplateId the ID of the selected template.
         */
        virtual void onNewSongParametersValidated(int selectedTemplateId) noexcept = 0;
        /** Called if the user canceled the new song creation. */
        virtual void onNewSongParametersCanceled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param idToTemplateName links an ID to the template name.
     * @param selectedTemplateId the ID of the selected template.
     */
    NewSongDialog(Listener& listener, std::map<int, juce::String> idToTemplateName, int selectedTemplateId) noexcept;

private:
    /** Called when the OK button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    Listener& listener;

    juce::Label templateLabel;
    juce::ComboBox templateComboBox;

    std::map<int, juce::String> idToTemplateName;
};

}   // namespace arkostracker
