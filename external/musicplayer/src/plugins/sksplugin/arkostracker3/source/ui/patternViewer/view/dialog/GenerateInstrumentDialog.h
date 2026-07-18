#pragma once

#include "../../../components/EditText.h"
#include "../../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

/** A dialog shown to generate an instrument from the selected block. */
class GenerateInstrumentDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param validateCallback the callback to validate, with the instrument name and a boolean to indicate whether to use periods instead of notes+shifts.
     * @param closeCallback called when the user canceled.
     */
    explicit GenerateInstrumentDialog(std::function<void(juce::String instrumentName, bool usePeriods)> validateCallback, std::function<void()> closeCallback) noexcept;

private:
    /** Called when the OK button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    std::function<void(juce::String instrumentName, bool usePeriods)> validateCallback;
    std::function<void()> closeCallback;

    juce::Label nameLabel;                              // Label for the name of the Instrument.
    EditText nameTextEditor;                            // Where the name of the Instrument can be written.

    juce::ToggleButton encodeAsPeriodsToggle;
    juce::ToggleButton encodeAsNotesToggle;
};

}   // namespace arkostracker
