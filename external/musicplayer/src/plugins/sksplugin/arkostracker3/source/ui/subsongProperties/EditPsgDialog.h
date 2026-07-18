#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../song/psg/Psg.h"
#include "../components/EditText.h"
#include "../components/dialogs/ModalDialog.h"
#include "../components/PsgFrequencyChooser.h"

namespace arkostracker 
{

/** A Dialog to shows the data of a PSG. */
class EditPsgDialog final : public ModalDialog
{
public:
    /** Listener to the events of this Dialog. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the PSG is sent. It may be the same.
         * @param psgIndex the index of the PSG.
         * @param psg the PSG data.
         */
        virtual void onPsgValidated(int psgIndex, Psg psg) noexcept = 0;
        /** Called once the user canceled the Dialog. */
        virtual void onPsgNotModified() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events of this class.
     * @param psg the PSG data to display.
     * @param psgIndex a convenience index, to be used for the callback.
     */
    EditPsgDialog(Listener& listener, const Psg& psg, int psgIndex) noexcept;

private:
    static const int offsetMixingOutputId;          // Because JUCE reserves ID 0.

    /** Called when the OK button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the Cancel button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /**
     * Fills the Views with the Given data.
     * @param psg the PSG data.
     */
    void fillViews(const Psg& psg) noexcept;

    /** Checks the Reference frequency value is valid, modifies it if necessary. This also selects the right Toggle. */
    void checkAndModifyReferencePsgFrequencyValueAndSelectIfNecessary() noexcept;
    /** Selects the Reference Frequency custom toggle. */
    void selectReferenceFrequencyCustomToggle() noexcept;

    /** Checks the Sample Player frequency value is valid, modifies it if necessary. */
    void checkAndModifySamplePlayerFrequencyValueAndSelectIfNecessary() noexcept;

    /** @return a PSG from the selected UI elements. */
    Psg buildPsgFromUi() const noexcept;

    /** Called when the "copy frequencies" button is clicked. */
    void onCopyFrequenciesButtonClicked() noexcept;

    Listener& listener;
    const int psgIndex;

    juce::GroupComponent psgTypeGroup;
    juce::ToggleButton ayToggle;
    juce::ToggleButton ymToggle;
    PsgFrequencyChooser psgFrequencyChooser;
    juce::GroupComponent referenceFrequencyGroup;
    juce::ToggleButton referenceFrequency440Toggle;
    juce::ToggleButton referenceFrequencyCustomToggle;
    EditText referenceFrequencyCustomTextEditor;

    juce::GroupComponent samplePlayerFrequencyGroup;
    EditText samplePlayerFrequencyEditor;

    juce::GroupComponent outputMixGroup;
    juce::ComboBox outputMixComboBox;

    juce::TextEditor::LengthAndCharacterRestriction restrictionInt;
    juce::TextEditor::LengthAndCharacterRestriction restrictionFloat;

    juce::TextButton copyFrequenciesButton;
    std::unique_ptr<juce::BubbleMessageComponent> messageBubble;
};

}   // namespace arkostracker
