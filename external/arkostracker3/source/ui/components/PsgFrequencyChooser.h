#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "EditText.h"
#include "../../utils/OptionalValue.h"

namespace arkostracker
{

/** Group with frequencies to choose. */
class PsgFrequencyChooser final : public juce::Component
{
public:
    static const int desiredHeightWithoutDontChangeToggle;
    static const int desiredHeightWithDontChangeToggle;

    /**
     * Constructor.
     * @param showDontChangeToggle true to show "don't change frequency".
     * @param groupTitle the title of the group.
     */
    PsgFrequencyChooser(bool showDontChangeToggle, const juce::String& groupTitle) noexcept;

    /**
     * Sets the frequency, selecting the right toggle accordingly.
     * @param frequencyHz the frequency, or absent to set to "don't force".
     */
    void setFrequencyHz(OptionalInt frequencyHz) noexcept;

    /** @return the selected frequency Hz, or absent if none is forced. */
    OptionalInt getSelectedFrequencyHz() const noexcept;

    // Component method implementations.
    // ==================================================
    void resized() override;

private:
    /** Checks the PSG frequency value is valid, modifies it if necessary. This also selects the right Toggle. */
    void checkAndModifyPsgFrequencyValueAndSelectIfNecessary() noexcept;

    bool showDontChangeToggle;
    juce::TextEditor::LengthAndCharacterRestriction restrictionInt;

    juce::GroupComponent psgFrequencyGroup;
    juce::ToggleButton psgFrequencyDontChangeToggle;
    juce::ToggleButton psgFrequencyCpcToggle;
    juce::ToggleButton psgFrequencySpectrumToggle;
    juce::ToggleButton psgFrequencyPentagonToggle;
    juce::ToggleButton psgFrequencyMsxToggle;
    juce::ToggleButton psgFrequencyAtariToggle;
    juce::ToggleButton psgFrequencyCustomToggle;
    EditText psgFrequencyCustomTextEditor;
};

}   // namespace arkostracker
