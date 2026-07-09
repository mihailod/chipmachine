#pragma once

#include "../../../components/EditText.h"
#include "../../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

/** A dialog shown to generate arpeggio from the selected notes. */
class GenerateArpeggioDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param validateCallback the callback to validate, with the arpeggio name and the base note chosen by the user.
     * @param closeCallback called when the user canceled.
     * @param notes the Arpeggio notes, for display.
     */
    explicit GenerateArpeggioDialog(std::function<void(juce::String arpeggioName, int baseNote)> validateCallback, std::function<void()> closeCallback,
                                    std::vector<int> notes) noexcept;

private:
    static constexpr auto baseNodeIdOffset = 1;          // JUCE can't handle ID 0.

    /** Called when the OK button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /** Fills the combobox with the base notes. */
    void fillBaseNoteComboBox() noexcept;

    std::function<void(juce::String arpeggioName, int baseNote)> validateCallback;
    std::function<void()> closeCallback;

    std::vector<int> notes;

    juce::Label nameLabel;                              // Label for the name of the Arpeggio.
    EditText nameTextEditor;                            // Where the name of the Arpeggio can be written.

    juce::Label baseNoteLabel;
    juce::ComboBox baseNoteComboBox;

    juce::Label noteCountLabel;
};

}   // namespace arkostracker
