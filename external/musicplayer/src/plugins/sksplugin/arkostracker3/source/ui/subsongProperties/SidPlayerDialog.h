#pragma once

#include "../../song/sid/SidPlayerCapability.h"
#include "../components/EditText.h"
#include "../components/dialogs/ModalDialog.h"

namespace arkostracker
{

class SidPlayerDialog final : public ModalDialog
{

public:
    /** Listener to the events of this Dialog. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the choice is made.
         * @param sidPlayerCapability the chosen capability.
         */
        virtual void onSidPlayerSelected(SidPlayerCapability sidPlayerCapability) noexcept = 0;
        /** Called once the user canceled the Dialog. */
        virtual void onSidPlayerCanceled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events of this class.
     * @param currentSidPlayerFrequency the current SID player frequency, to show in the UI.
     */
    SidPlayerDialog(Listener& listener, const SidPlayerCapability& currentSidPlayerFrequency) noexcept;

private:
    /** Called when the OK button is clicked. */
    void onOkButtonClicked() noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /** The textEdit must be enabled/disabled. */
    void onCustomToggleStateChanged() noexcept;

    /** Corrects the fields, in the UI. */
    void correctFields() noexcept;
    void onTextFieldChanged() noexcept;

    Listener& listener;
    juce::TextEditor::LengthAndCharacterRestriction restrictionIntFrequency;
    juce::TextEditor::LengthAndCharacterRestriction restrictionIntPeriods;

    juce::Label topLabel;
    juce::ToggleButton cpcToggle;
    juce::ToggleButton amstradPlusToggle;
    juce::ToggleButton atariStToggle;
    juce::ToggleButton customToggle;
    juce::Label customFrequencyLabel;
    EditText customFrequencyTextEditor;
    juce::Label customMinimumPeriodLabel;
    EditText customMinimumPeriodTextEditor;
    juce::Label customMaximumPeriodLabel;
    EditText customMaximumPeriodTextEditor;
};

}   // namespace arkostracker
