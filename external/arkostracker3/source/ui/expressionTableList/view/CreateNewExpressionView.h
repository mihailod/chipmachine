#pragma once

#include <memory>

#include "../../components/EditText.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../controller/ExpressionTemplate.h"

namespace arkostracker 
{

/** Shows a Dialog indicating the name of the new Expression. */
class CreateNewExpressionView
{
public:
    /** Listener to the events of this class. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user has validated the name of the expression to create.
         * @param index the index that was given when creating this object, as a convenience.
         * @param name the name.
         * @param desiredTemplate the desired template (arpeggio or pitch).
         */
        virtual void onCreateNewExpressionValidated(int index, juce::String name, ExpressionTemplate::Template desiredTemplate) noexcept = 0;

        /** Called when the user cancelled the creation of the expression. */
        virtual void onCreateNewExpressionCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param index a convenience index to be stored, then returned by the listener.
     * @param isArpeggio true if arpeggio, false if pitch.
     */
    CreateNewExpressionView(Listener& listener, int index, bool isArpeggio) noexcept;

private:
    static constexpr auto offsetTemplateId = 1;              // 0 as an ID is forbidden by JUCE.

    /** Called when the OK Button dialog has been clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called if the dialog has been cancelled. */
    void onDialogCancelled() const noexcept;

    /** Fills the Template ComboBox. */
    void fillComboBox() noexcept;

    Listener& listener;
    bool isArpeggio;
    const int index;

    std::unique_ptr<ModalDialog> modalDialog;

    juce::Label nameLabel;
    EditText nameTextEditor;

    juce::Label templateLabel;
    juce::ComboBox templateComboBox;
};

}   // namespace arkostracker
